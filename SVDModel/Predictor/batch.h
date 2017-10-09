#ifndef BATCH_H
#define BATCH_H

#include <list>
//#include "tensorhelper.h"
#include "inferencedata.h"

class BatchManager; // forward

class Batch
{
public:
    Batch(int batch_size);
    ~Batch();
    enum BatchState { Fill=0, DNN=1, Finished=2};
    BatchState state() const { return mState; }
    BatchState changeState(BatchState newState);

    /// get slot number in the batch (atomic access)
    int acquireSlot();
    /// number of slots that are free
    int freeSlots();
    /// number of slots currently in use
    int usedSlots() { return mCurrentSlot; }

    /// get a specific tensor from the batch
    /// the 'index' is stored in the tensor definition.
    TensorWrapper *tensor(int index) {return mTensors[index]; }

    /// access to the InferenceData
    InferenceData &inferenceData(size_t slot) { if (slot<mInferenceData.size()) return mInferenceData[slot];
        throw std::logic_error("Batch: invalid slot!");}

private:
    BatchState mState;
    std::vector<InferenceData> mInferenceData;
    /// the tensors associated with this batch of data
    std::vector<TensorWrapper*> mTensors;
    std::atomic<int> mCurrentSlot; ///< atomic access; number of currently used slots (not the index!)
    int mBatchSize;
    friend class BatchManager;
};

#endif // BATCH_H
