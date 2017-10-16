#ifndef INFERENCEDATA_H
#define INFERENCEDATA_H
#include "cell.h"
#include "states.h"
#include "batchmanager.h"
#include "environmentcell.h"
class Batch; // forward

class InferenceData
{
public:
    InferenceData(): mOldState(-1), mNextState(-1), mNextTime(-1), mBatch(nullptr), mSlot(-1) {}
    /// fills an item by pulling all the required data from the model
    void fetchData(Cell *cell, Batch *batch, int slot);

    /// set the result of the DNN
    void setResult(state_t state, restime_t time);
    state_t nextState() const { return mNextState; }
    restime_t nextTime() const { return mNextTime; }
    /// write the result back to the cell in the model
    void writeResult();

    // access
    const EnvironmentCell &environmentCell() const;

    /// get a human readable string from the data in the tensors for the example in 'slot'
    std::string dumpTensorData();
private:
    /// pull the data from the model and stores in the Tensor
    void internalFetchData();
    // functions for individual content types
    void fetchClimate(const InputTensorItem &def);
    void fetchState(const InputTensorItem &def);
    void fetchResidenceTime(const InputTensorItem &def);
    void fetchNeighbors(const InputTensorItem &def);
    void fetchSite(const InputTensorItem &def);
    state_t mOldState;
    state_t mNextState;
    restime_t mNextTime;
    restime_t mResidenceTime;
    int mIndex; ///< index of the cell in the landscape grid
    Batch *mBatch; ///< link to the batch (that contains the actual Tensors)
    int mSlot; ///< slot within the batch for this cell
};

#endif // INFERENCEDATA_H
