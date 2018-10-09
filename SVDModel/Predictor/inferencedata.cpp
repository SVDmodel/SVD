#include "inferencedata.h"
#include "model.h"
#include "batchdnn.h"
#include "tensorhelper.h"
#include "dnn.h"

bool InferenceData::checkSetup(const InputTensorItem &def)
{

    switch (def.content) {
    case InputTensorItem::SiteNPKA:
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
    //internalFetchData();
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

