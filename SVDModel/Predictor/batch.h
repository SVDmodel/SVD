#ifndef BATCH_H
#define BATCH_H

#include <list>
#include "tensorhelper.h"
class BatchManager; // forward

class Batch
{
public:
    Batch(int batch_size);
    ~Batch();
    /// get slot number in the batch (atomic access)
    int acquireSlot();
    /// number of slots that are free
    int freeSlots();

    /// get a specific tensor from the batch
    /// the 'index' is stored in the tensor definition.
    TensorWrapper *tensor(int index) {return mTensors[index]; }

private:
    /// the tensors associated with this batch of data
    std::vector<TensorWrapper*> mTensors;
    std::atomic<int> mCurrentSlot; ///< atomic access
    int mBatchSize;
    friend class BatchManager;
};

#endif // BATCH_H
