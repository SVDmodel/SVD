#include "batch.h"
#include <atomic>

#include "spdlog/spdlog.h"


Batch::Batch(size_t batch_size)
{

    mCurrentSlot = 0;
    mCellsFinished = 0;
    mBatchSize = batch_size;
    mState=Fill;
    mError=false;
    mType = Invalid;
    mModule = nullptr;
    mPackageId=0;
    mCells.resize(mBatchSize);
}

Batch::~Batch()
{
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

size_t Batch::acquireSlot()
{
    // use an atomic operation
    size_t slot = mCurrentSlot.fetch_add(1); // read first, than add 1
    if (slot >= mBatchSize)
        throw std::logic_error("Batch::acquireSlot: batch full!");
    return slot;
}

size_t Batch::freeSlots()
{
    size_t slot = mCurrentSlot;
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

void Batch::processResults()
{
    spdlog::get("main")->debug("Batch::processResults: base class called (something is missing in derived class?)");
}
