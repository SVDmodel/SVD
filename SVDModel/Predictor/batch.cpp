#include "batch.h"
#include <atomic>

#include "spdlog/spdlog.h"

Batch::Batch(int batch_size)
{
    // dummy
    TensorWrap2d<float> test(10, 2);
    m2dTensors.push_back(&test);

    tensorflow::Tensor &t = m2dTensors.back()->tensor();
    spdlog::get("main")->debug("{}", t.dims());

    switch (m2dTensors.back()->ndim()) {
    case 2:
        spdlog::get("main")->debug("2d"); break;
    case 3:
        spdlog::get("main")->debug("3d"); break;
    default:
        spdlog::get("main")->debug("invalid"); break;
    }

    mCurrentSlot = -1;
    mBatchSize = batch_size;
}

int Batch::acquireSlot()
{
    // use an atomic operation
    int slot = mCurrentSlot.fetch_add(1);
    if (slot >= mBatchSize-1)
        throw std::logic_error("Batch::acquireSlot: batch full!");
    return slot;
}

int Batch::freeSlots()
{
    int slot = mCurrentSlot;
    return mBatchSize - slot - 1;
}
