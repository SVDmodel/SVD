#include "batchmanager.h"
#include "batchdnn.h"
#include "tensorhelper.h"

#include "model.h"
#include "settings.h"
#include "modules/module.h"
#include "filereader.h"
#include "tools.h"

#include <mutex>


#include "strtools.h"

BatchManager *BatchManager::mInstance = nullptr;







BatchManager::BatchManager()
{
    if (mInstance!=nullptr)
        throw std::logic_error("Creation of batch manager: instance ptr is not 0.");
    mInstance = this;
    if (spdlog::get("dnn"))
        spdlog::get("dnn")->debug("Batch manager created: {}", static_cast<void*>(this));

}

BatchManager::~BatchManager()
{
    // delete all batches and free memory
    for (auto b : mBatches)
        delete b;

    if (auto lg = spdlog::get("dnn"))
        lg->debug("Batch manager destroyed: {x}", static_cast<void*>(this));

    mInstance = nullptr;
}

void BatchManager::setup()
{

    lg = spdlog::get("dnn");
    if (!lg)
        throw std::logic_error("BatchManager::setup: logging not available.");
    lg->info("Setup of batch manager.");
    Model::instance()->settings().requiredKeys("dnn", {"batchSize", "maxBatchQueue", "metadata"});
    mBatchSize = Model::instance()->settings().valueUInt("dnn.batchSize");
    mMaxQueueLength = Model::instance()->settings().valueUInt("dnn.maxBatchQueue");


}

void BatchManager::newYear()
{
    mSlotRequested = false;
}

static std::mutex batch_mutex;
std::pair<Batch *, size_t> BatchManager::validSlot(Module *module)
{
    // serialize this function...
    std::lock_guard<std::mutex> guard(batch_mutex);
    mSlotRequested = true;
    std::pair<Batch *, size_t> result;
    int sleeps = 0;
    do {
        result = findValidSlot(module);
        if (!result.first) {
            // wait


            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            Model::instance()->processEvents();

            if (++sleeps % 100 == 0) // 1s
                lg->trace("BatchManager: no batch available (queue full). Sleeping for {} s.", sleeps/100);

            if (RunState::instance()->cancel() ) {
                lg->info("Canceled.");
                return std::pair<Batch*, int>(nullptr, 0);

            }
            if ( sleeps % 10000 == 0) { // 100 secs
                lg->error("time out in batch manager - no empty slots found.");
                return std::pair<Batch*, int>(nullptr, -1);

            }
        }
    } while (!result.first);
    return result;

}

BatchDNN *BatchManager::createDNNBatch()
{
    BatchDNN *b = new BatchDNN(mBatchSize);

    return b;

}

std::pair<Batch *, size_t> BatchManager::findValidSlot(Module *module)
{
    // this function is serialized (access via validSlot() ).

    // look for a batch which is currently not in the DNN processing chain
    Batch *batch = nullptr;
    for (const auto &b : mBatches) {
        if (b->module()==module && b->state()==Batch::Fill && b->freeSlots()>0) {
            batch=b;
            break;
        }
    }
    if (!batch || batch->freeSlots()<=0) {
        if (mBatches.size() >= mMaxQueueLength) {
            // currently we don't find a proper place for the data.
            return std::pair<Batch*, size_t>(nullptr, 0);
        }
        // create a new batch; the default (forest) is a batch for DNN
        batch = createBatch(module ? module->batchType() : Batch::DNN);
        batch->setModule(module);
        mBatches.push_back( batch );
        lg->trace("created a new batch. Now the list contains {} batch(es).", mBatches.size());
        /*if ( lg->should_log(spdlog::level::trace) ) {
            int idx=0;
            for (auto b : mBatches) {
                lg->trace("#{}: state: {}, used: {}, free: {}", idx, b->state(), b->usedSlots(), b->freeSlots());
                ++idx;
            }
        }*/
    }


    // get a new slot in the batch
    std::pair<Batch *, size_t> result;
    result.first = batch;
    result.second = batch->acquireSlot();
    if (result.second==0) {
        lg->trace("Started to fill batch [{}] (first slot acquired)", static_cast<void*>(batch));
    }
    return result;


}


Batch *BatchManager::createBatch(Batch::BatchType type)
{
    Batch *b = nullptr;
    switch (type) {
    case Batch::DNN:
        b = createDNNBatch();
        break;
    case Batch::Simple:
        b = new Batch(mBatchSize);
        break;
    default: throw std::logic_error("BatchManager:createBatch: invalid batch type!");
    }

    return b;
}


