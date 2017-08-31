#ifndef BATCH_H
#define BATCH_H

#include <list>
#include "tensorhelper.h"

class Batch
{
public:
    Batch(int batch_size);
    /// get slot number in the batch (atomic access)
    int acquireSlot();
    /// number of slots that are free
    int freeSlots();
private:
    std::list<TensorWrapper*> m2dTensors;
    std::atomic<int> mCurrentSlot;
    int mBatchSize;
};

#endif // BATCH_H
