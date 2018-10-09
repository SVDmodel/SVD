#ifndef INFERENCEDATA_H
#define INFERENCEDATA_H
#include "cell.h"
#include "states.h"
#include "batchmanager.h"
#include "environmentcell.h"
class BatchDNN; // forward
class Batch; // forward

class InferenceData
{
public:
    InferenceData(): mOldState(-1), mNextState(-1), mNextTime(-1), mBatch(nullptr), mSlot(std::numeric_limits<size_t>::max()) {}
    /// check if for the given item all the data is avaialable during setup
    static bool checkSetup(const InputTensorItem &def);

    /// fills an item by pulling all the required data from the model
    void fetchData(Cell *cell, BatchDNN *batch, size_t slot);

    /// set the result of the DNN
    void setResult(state_t state, restime_t time);
    state_t nextState() const { return mNextState; }
    state_t state() const { return mOldState; }
    /// the absolute time (year) the next evaluation / state change will happen.
    restime_t nextTime() const { return mNextTime; }
    /// write the result back to the cell in the model
    void writeResult();

    // access
    const EnvironmentCell &environmentCell() const;
    const Cell &cell() const;

    /// get a human readable string from the data in the tensors for the example in 'slot'
    std::string dumpTensorData();
private:
    state_t mOldState;
    state_t mNextState;
    restime_t mNextTime;
    restime_t mResidenceTime;
    int mIndex; ///< index of the cell in the landscape grid
    BatchDNN *mBatch; ///< link to the batch (that contains the actual Tensors)
    size_t mSlot; ///< slot within the batch for this cell
};

#endif // INFERENCEDATA_H
