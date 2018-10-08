#include "inferencedata.h"
#include "model.h"
#include "batchdnn.h"
#include "tensorhelper.h"
#include "dnn.h"

bool InferenceData::checkSetup(const InputTensorItem &def)
{

    switch (def.content) {
    case InputTensorItem::Site:
        // required columns: availableNitrogen, soilDepth
        if (!contains(EnvironmentCell::variables(), "availableNitrogen") || !contains(EnvironmentCell::variables(), "soilDepth")) {
            spdlog::get("dnn")->error("The required columns 'availableNitrogen' and 'soilDepth' are not available in the environment (item: '{}', available: '{}').", def.name, join(EnvironmentCell::variables()));
            return false;
        }
        return true;
    case InputTensorItem::DistanceOutside:
            if (!contains(EnvironmentCell::variables(), "distanceOutside") ) {
                spdlog::get("dnn")->error("The required columns 'distanceOutside' is not available in the environment (item: '{}', available: '{}').", def.name, join(EnvironmentCell::variables()));
                return false;
            }
            return true;
    default:
        return true; // no specific tests for this item
    }

}

void InferenceData::fetchData(Cell *cell, BatchDNN *batch, size_t slot)
{
    mOldState = cell->stateId();
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
    if (state==0)
        spdlog::get("main")->error("InferenceData::setResult, state=0!");
    mNextState=state;
    // time is the number of years the next update should happen
    // we change to the absolute year:
    mNextTime=static_cast<restime_t>(Model::instance()->year() + time);
}

void InferenceData::writeResult()
{
    // write back:
    Cell &cell = Model::instance()->landscape()->grid()[mIndex];
    cell.setNextStateId(mNextState);
    cell.setNextUpdateTime(mNextTime);


}

const EnvironmentCell &InferenceData::environmentCell() const
{
    return Model::instance()->landscape()->environmentCell(mIndex);
}

const Cell &InferenceData::cell() const
{
    return Model::instance()->landscape()->grid()[mIndex];
}

std::string InferenceData::dumpTensorData()
{
    std::stringstream ss;
    const std::list<InputTensorItem> &tdef = DNN::instance()->tensorDefinition();
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
    const std::list<InputTensorItem> &tdef = DNN::instance()->tensorDefinition();
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
        case InputTensorItem::DistanceOutside:
            fetchDistanceOutside(def);
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
    // stateId starts with 1, the state tensor is 0-based
    *p = mOldState - 1;

}

void InferenceData::fetchResidenceTime(const InputTensorItem &def)
{
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(mSlot);
    // TODO: residence time, now fixed divide by 10
    *p = static_cast<float>(mResidenceTime / 10.f);
}

void InferenceData::fetchNeighbors(const InputTensorItem &def)
{
    const size_t n_neighbors = 62; // 2x32
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(mSlot);

    auto &ec = Model::instance()->landscape()->cell(mIndex);
    auto neighbors = ec.neighborSpecies();
    if (neighbors.size() != n_neighbors)
        throw std::logic_error("Invalid number of neighbors...");

    for (size_t i=0;i<n_neighbors;++i)
        *p++ = static_cast<float>(neighbors[i]);

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

void InferenceData::fetchDistanceOutside(const InputTensorItem &def)
{
    const int i_distance = 2;
    TensorWrapper *t = mBatch->tensor(def.index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(mSlot);
    auto &ec = Model::instance()->landscape()->environmentCell(mIndex);

    *p = static_cast<float>( ec.value(i_distance) );

}
