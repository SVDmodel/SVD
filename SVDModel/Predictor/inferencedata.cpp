#include "inferencedata.h"
#include "model.h"
#include "batch.h"
#include "tensorhelper.h"

void InferenceData::fetchData(Cell *cell, Batch *batch, int slot)
{
    mOldState = cell->state();
    mNextState = 0;
    mNextTime = 0;
    mIndex = cell - Model::instance()->landscape()->currentGrid().begin();

    mBatch = batch;
    mSlot = slot;

    // now pull all the data
    internalFetchData();
}

void InferenceData::writeResult()
{
    // write back:
    Cell &cell = Model::instance()->landscape()->futureGrid()[size_t(mIndex)];
    cell.setState(mNextState);
    cell.setNextUpdateTime(mNextTime);
    cell.setResidenceTime(0);

}

void InferenceData::internalFetchData()
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
    auto &ec = Model::instance()->landscape()->environmentCell(mIndex);
    auto climate_series = Model::instance()->climate()->series(Model::instance()->year(),
                                                               def.sizeY,
                                                               ec.climateId());
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap3d<float> *tw = static_cast<TensorWrap3d<float>*>(t);

    //return;
    // copy the climate data to the tensors
    size_t i = 0;
    for (const std::vector<float> *p : climate_series) {
        float *d = tw->row(mSlot, i++);
        memcpy(d, p->data(), sizeof(float) * p->size());
    }

}

void InferenceData::fetchState(const InputTensorItem &def)
{
    // the current state
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap2d<short unsigned int> *tw = static_cast<TensorWrap2d<short unsigned int>*>(t);
    short unsigned int *p = tw->example(mSlot);
    //return;
    if (p - tw->example(0) >= tw->batchSize())
        throw std::logic_error("fetchState!");
    *p = mOldState;

}

void InferenceData::fetchResidenceTime(const InputTensorItem &def)
{

}
