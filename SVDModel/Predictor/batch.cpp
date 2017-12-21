#include "batch.h"
#include <atomic>

#include "spdlog/spdlog.h"

#include "tensorhelper.h"

Batch::Batch(int batch_size)
{

    mCurrentSlot = 0;
    mCellsFinished = 0;
    mBatchSize = batch_size;
    mInferenceData.resize(mBatchSize);
    mState=Fill;
    mError=false;
}

Batch::~Batch()
{
    // free the memory of the tensors...
    if (spdlog::get("dnn"))
        spdlog::get("dnn")->trace("Destructor of batch, free tensors");
    for (auto p : mTensors) {
        delete p;
    }
}

Batch::BatchState Batch::changeState(Batch::BatchState newState)
{
    if (newState==Fill) {
        mCurrentSlot = 0;
        mCellsFinished = 0;
        mState = newState;
    } else {
        mState = newState;
    }
    return mState;
}

int Batch::acquireSlot()
{
    // use an atomic operation
    int slot = mCurrentSlot.fetch_add(1); // read first, than add 1
    if (slot >= mBatchSize)
        throw std::logic_error("Batch::acquireSlot: batch full!");
    return slot;
}

int Batch::freeSlots()
{
    int slot = mCurrentSlot;
    return mBatchSize - slot;
}

void Batch::finishedCellProcessing()
{
    ++mCellsFinished;
}

bool Batch::allCellsProcessed()
{
    // a batch is done if no free slots are available and all slots are processed
    if (freeSlots()<=0 && mCellsFinished==usedSlots())
        return true;

    return false;
}
