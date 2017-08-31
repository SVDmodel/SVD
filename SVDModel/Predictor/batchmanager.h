#ifndef BATCHMANAGER_H
#define BATCHMANAGER_H

#include <utility>
#include <list>
#include <cassert>

class Batch;  // forward

class BatchManager
{
public:
    BatchManager();
    ~BatchManager();
    /// access to the currently avaialable BatchManager
    /// this allows accessing the model with BatchManager::instance()->....
    static BatchManager *instance() {
        assert(mInstance!=nullptr);
        return mInstance; }

    /// returns a pointer to a batch (first) and a (valid)
    /// slot (=index within the batch): second
    std::pair<Batch *, int> batch();
private:
    Batch *createBatch();
    std::list<Batch *> mBatches;
    static BatchManager *mInstance;
};

#endif // BATCHMANAGER_H
