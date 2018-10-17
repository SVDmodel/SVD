#ifndef BATCHMANAGER_H
#define BATCHMANAGER_H

#include <utility>
#include <list>
#include <cassert>
#include <memory>
#include "spdlog/spdlog.h"

#include "batch.h"
#include "inputtensoritem.h"

class BatchDNN;  // forward
class TensorWrapper; // forward
class Module; // forward



class BatchManager
{
public:
    BatchManager();
    ~BatchManager();
    void setup();
    /// called at the beginning of a year
    void newYear();

    /// access to the currently avaialable BatchManager
    /// this allows accessing the model with BatchManager::instance()->....
    static BatchManager *instance() {
        assert(mInstance!=nullptr);
        return mInstance; }

    size_t batchSize() const { return mBatchSize; }

    std::shared_ptr<spdlog::logger> &log() {return lg; }

    /// returns a pointer to a batch (first) and a (valid)
    /// slot (=index within the batch): second
    std::pair<Batch *, size_t> validSlot(Module *module);

    const std::list<Batch *> batches() const { return mBatches; }

    bool slotsRequested() const { return mSlotRequested; }

private:
    size_t mBatchSize;
    size_t mMaxQueueLength;
    bool mSlotRequested;
    BatchDNN *createDNNBatch();
    Batch *createBatch(Batch::BatchType type);
    std::pair<Batch *, size_t> findValidSlot(Module *module);
    std::list<Batch *> mBatches;
    static BatchManager *mInstance;

    // logging
    std::shared_ptr<spdlog::logger> lg;
};

#endif // BATCHMANAGER_H
