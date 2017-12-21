#ifndef BATCH_H
#define BATCH_H

#include <list>

#include "inferencedata.h"

class BatchManager; // forward

class Batch
{
public:
    Batch(int batch_size);
    ~Batch();

    /// the state of the batch
    enum BatchState { Fill=0, DNN=1, Finished=2, FinishedDNN=3};
    BatchState state() const { return mState; }
    BatchState changeState(BatchState newState);

    bool hasError() const { return mError; }
    void setError(bool error) { mError = error; }

    int packageId() const { return mPackageId; }
    void setPackageId(int id) { mPackageId = id; }
    int batchSize() const { return mBatchSize; }

    /// get slot number in the batch (atomic access)
    int acquireSlot();
    /// number of slots that are free
    int freeSlots();
    /// number of slots currently in use
    int usedSlots() { return mCurrentSlot; }

    /// is called when a cell is finished (decrease the atomic counter)
    void finishedCellProcessing();

    /// returns true if the batch is full and all cells are processed
    bool allCellsProcessed();

    /// get a specific tensor from the batch
    /// the 'index' is stored in the tensor definition.
    TensorWrapper *tensor(int index) {return mTensors[index]; }

    /// access to the InferenceData
    InferenceData &inferenceData(size_t slot) { if (slot<mInferenceData.size()) return mInferenceData[slot];
        throw std::logic_error("Batch: invalid slot!");}

private:
    bool mError;
    BatchState mState;
    std::vector<InferenceData> mInferenceData;
    /// the tensors associated with this batch of data
    std::vector<TensorWrapper*> mTensors;
    std::atomic<int> mCurrentSlot; ///< atomic access; number of currently used slots (not the index!)
    std::atomic<int> mCellsFinished; ///< number of cells which already finished during the "filling"
    int mBatchSize;
    int mPackageId;
    friend class BatchManager;
};

#endif // BATCH_H
