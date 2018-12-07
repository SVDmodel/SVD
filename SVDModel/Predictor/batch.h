/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
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
