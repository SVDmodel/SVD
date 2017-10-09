#ifndef INFERENCEDATA_H
#define INFERENCEDATA_H
#include "cell.h"
#include "states.h"
#include "batchmanager.h"
class Batch; // forward

class InferenceData
{
public:
    InferenceData(): mOldState(-1), mNextState(-1), mNextTime(-1), mBatch(nullptr), mSlot(-1) {}
    /// fills an item by pulling all the required data from the model
    void fetchData(Cell *cell, Batch *batch, int slot);

    void setResult(state_t state, restime_t time) { mNextState=state; mNextTime=time; }
    void writeResult();
private:
    /// pull the data from the model and stores in the Tensor
    void internalFetchData();
    // functions for individual content types
    void fetchClimate(const InputTensorItem &def);
    void fetchState(const InputTensorItem &def);
    void fetchResidenceTime(const InputTensorItem &def);
    state_t mOldState;
    state_t mNextState;
    restime_t mNextTime;
    size_t mIndex; ///< index of the cell in both landscape grids
    Batch *mBatch; ///< link to the batch (that contains the actual Tensors)
    int mSlot; ///< slot within the batch for this cell
};

#endif // INFERENCEDATA_H
