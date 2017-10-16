#include "inferencedata.h"
#include "model.h"
#include "batch.h"
#include "tensorhelper.h"

void InferenceData::fetchData(Cell *cell, Batch *batch, int slot)
{
    mOldState = cell->state();
    mResidenceTime = cell->residenceTime();
    mNextState = 0;
    mNextTime = 0;
    mIndex = static_cast<int>( cell - Model::instance()->landscape()->grid().begin() );

    mBatch = batch;
    mSlot = slot;

    // now pull all the data
    internalFetchData();
}

void InferenceData::setResult(state_t state, restime_t time)
{
    mNextState=state;
    mNextTime=Model::instance()->year() + time;
}

void InferenceData::writeResult()
{
    // write back:
    Cell &cell = Model::instance()->landscape()->grid()[mIndex];
    cell.setNextState(mNextState);
    cell.setNextUpdateTime(mNextTime);


}

const EnvironmentCell &InferenceData::environmentCell() const
{
    return Model::instance()->landscape()->environmentCell(mIndex);
}

std::string InferenceData::dumpTensorData()
{
    std::stringstream ss;
    const std::list<InputTensorItem> &tdef = BatchManager::instance()->tensorDefinition();
    ss << " **** Dump for example " << mSlot << " **** " << std::endl;
    for (const auto &def : tdef) {
        ss << "*****************************************" << std::endl;
        ss << "Tensor: '" << def.name << "', ";
        ss << "dtype: " << InputTensorItem::datatypeString(def.type) << ", Dimensions: " << def.ndim;
        if (def.ndim == 1)
            ss << ", Size: " << def.sizeX << std::endl;
        else
            ss << ", Size: "<< def.sizeX << " x " << def.sizeY << std::endl;
        ss << "*****************************************" << std::endl;
        ss <<  mBatch->tensor(def.index)->asString(mSlot) << std::endl;

    }
    return ss.str();
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
        case InputTensorItem::Site:
            fetchSite(def);
            break;
        case InputTensorItem::Neighbors:
            fetchNeighbors(def);
            break;
        case InputTensorItem::Scalar:
            // a scalar is already set to the correct value.
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
                                                               def.sizeX,
                                                               ec.climateId());
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap3d<float> *tw = static_cast<TensorWrap3d<float>*>(t);

    if (climate_series.size() != def.sizeX || climate_series[0]->size() != def.sizeY)
        throw std::logic_error("InferenceData::fetchClimate: mismatch in dimensions: expected " +
                               to_string(def.sizeX) + ", got " + to_string(climate_series.size()) +  " years; " +
                               "expected " + to_string(def.sizeY) + ", got " + to_string(climate_series[0]->size()) + " columns (per year)!");
    //return;
    // copy the climate data to the tensors
    // TODO: transform inputs
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
    TensorWrap2d<short int> *tw = static_cast<TensorWrap2d<short int>*>(t);
    short int *p = tw->example(mSlot);
    *p = mOldState;

}

void InferenceData::fetchResidenceTime(const InputTensorItem &def)
{
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(mSlot);
    *p = static_cast<float>(mResidenceTime);
}

void InferenceData::fetchNeighbors(const InputTensorItem &def)
{
    const size_t n_neighbors = 62;
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(mSlot);

    // TODO
    for (size_t i=0;i<n_neighbors;++i)
        *p++ = 0.f;

}

void InferenceData::fetchSite(const InputTensorItem &def)
{
    const int i_nitrogen=0, i_soildepth=1;
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(mSlot);
    // site: nitrogen/soil-depth
    auto &ec = Model::instance()->landscape()->environmentCell(mIndex);
    // TODO: transformation...
    *p++ = static_cast<float>( (ec.value(i_nitrogen) -58.500)/41.536 );
    *p++ = static_cast<float>( (ec.value(i_soildepth)-58.500)/41.536 );
}
