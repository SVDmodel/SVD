#include "batch.h"
#include <atomic>

#include "spdlog/spdlog.h"

Batch::Batch(int batch_size)
{

    mCurrentSlot = 0;
    mBatchSize = batch_size;
}

Batch::~Batch()
{
    // free the memory of the tensors...
    spdlog::get("dnn")->trace("Destructor of batch, free tensors");
    for (auto p : mTensors) {
        delete p;
    }
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
