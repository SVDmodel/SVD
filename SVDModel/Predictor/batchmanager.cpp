#include "batchmanager.h"
#include "batch.h"

#include <mutex>

#include "spdlog/spdlog.h"


BatchManager *BatchManager::mInstance = 0;

BatchManager::BatchManager()
{
    if (mInstance!=nullptr)
        throw std::logic_error("Creation of batch manager: instance ptr is not 0.");
    mInstance = this;
    if (spdlog::get("dnn"))
        spdlog::get("dnn")->debug("Batch manager created: {0:x}", (void*)this);

}

BatchManager::~BatchManager()
{
    if (spdlog::get("dnn"))
        spdlog::get("dnn")->debug("Batch manager destroyed: {0:x}", (void*)this);

    mInstance = nullptr;
}

std::mutex batch_mutex;
std::pair<Batch *, int> BatchManager::batch()
{
    // serialize this function...
    std::lock_guard<std::mutex> guard(batch_mutex);

    if (mBatches.size()==0)
        mBatches.push_back( createBatch() );

    if (mBatches.back()->freeSlots()<=0) {
        // we need to create a new batch
        mBatches.push_back( createBatch() );
    }

    Batch *last = mBatches.back();

    // get a new slot in the batch
    std::pair<Batch *, int> result;
    result.first = last;
    result.second = last->acquireSlot();
    return result;

}

Batch *BatchManager::createBatch()
{
    const int batch_size=16;

    Batch *b = new Batch(batch_size);

    return b;
}
