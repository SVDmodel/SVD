#ifndef BATCH_H
#define BATCH_H

#include <list>
#include <atomic>
#include <vector>

//#include "inferencedata.h"

class BatchManager; // forward
class Cell; // forward
class Module; // forward

class Batch
{
public:
    Batch(size_t batch_size);
    virtual ~Batch();

    /// the state of the batch
    enum BatchState { Fill=0, DNNInference=1, Finished=2, FinishedDNN=3};
    enum BatchType { Invalid=0, DNN=1, Simple=2 };
    BatchType type() const { return mType; }
    BatchState state() const { return mState; }
    BatchState changeState(BatchState newState);

    bool hasError() const { return mError; }
    void setError(bool error) { mError = error; }

    int packageId() const { return mPackageId; }
    void setPackageId(int id) { mPackageId = id; }
    void setModule(Module *module) { mModule = module; }
    Module *module() const { return mModule; }
    size_t batchSize() const { return mBatchSize; }

    /// get slot number in the batch (atomic access)
    size_t acquireSlot();
    /// number of slots that are free
    size_t freeSlots();
    /// number of slots currently in use
    size_t usedSlots() { return mCurrentSlot; }

    void setCell(Cell* cell, size_t slot) { mCells[slot] = cell; }
    const std::vector<Cell*> &cells() const { return mCells; }

    /// is called when a cell is finished (decrease the atomic counter)
    void finishedCellProcessing();

    /// returns true if the batch is full and all cells are processed
    bool allCellsProcessed();

    virtual void processResults();


protected:
    bool mError;
    BatchState mState;
    BatchType mType;
    std::atomic<size_t> mCurrentSlot; ///< atomic access; number of currently used slots (not the index!)
    std::atomic<size_t> mCellsFinished; ///< number of cells which already finished during the "filling"
    size_t mBatchSize;
    int mPackageId;
    /// minimal storage: the cells
    std::vector< Cell* > mCells;
    /// the handling module if present
    Module *mModule;
    friend class BatchManager;
};

#endif // BATCH_H
