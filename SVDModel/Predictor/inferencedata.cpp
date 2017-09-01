#include "inferencedata.h"
#include "model.h"
#include "batch.h"

InferenceData::InferenceData(Cell *cell)
{
    mOldState = cell->state();
    mNextState = 0;
    mNextTime = 0;
    mIndex = cell - Model::instance()->landscape()->currentGrid().begin();

    // get a batch to store the data to:
    assert(BatchManager::instance()!=nullptr);
    std::pair<Batch*, int> newslot = BatchManager::instance()->batch();
    mBatch = newslot.first;
    mSlot = newslot.second;

    // now pull all the data
    fetchData();
}

void InferenceData::writeResult()
{
    // write back:
    Cell &cell = Model::instance()->landscape()->futureGrid()[size_t(mIndex)];
    cell.setState(mNextState);
    cell.setNextUpdateTime(mNextTime);
    cell.setResidenceTime(0);

}

void InferenceData::fetchData()
{
    const std::list<InputTensorItem> &tdef = BatchManager::instance()->tensorDefinition();
    for (const auto &def : tdef) {
        switch (def.content) {
        case InputTensorItem::Climate:
            fetchClimate(def);
            break;
        case InputTensorItem::State:
            fetchState(def);
            break;
        case InputTensorItem::ResidenceTime:
            fetchResidenceTime(def);
            break;
        default:
            throw std::logic_error("InferenceData::fetchData: invalid content type.");

        }
    }
}

void InferenceData::fetchClimate(const InputTensorItem &def)
{
    // the climate data
    auto ec = Model::instance()->landscape()->environmentCell(mIndex);
    auto climate_series = Model::instance()->climate()->series(Model::instance()->year(),
                                                               def.sizeY,
                                                               ec.climateId());
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap3d<float> *tw = static_cast<TensorWrap3d<float>*>(t);

    // copy the climate data to the tensors
    size_t i = 0;
    for (auto p : climate_series) {
        float *d = tw->row(mSlot, i++);
        memcpy(d, p->data(), sizeof(float) * p->size());
    }

}

void InferenceData::fetchState(const InputTensorItem &def)
{
    // the current state
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap2d<unsigned int> *tw = static_cast<TensorWrap2d<unsigned int>*>(t);
    unsigned int *p = tw->example(mSlot);
    *p = mOldState;

}

void InferenceData::fetchResidenceTime(const InputTensorItem &def)
{

}
