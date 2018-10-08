#ifndef BATCHDNN_H
#define BATCHDNN_H

#include "batch.h"
#include "inferencedata.h"

class BatchDNN : public Batch
{
public:
    BatchDNN(size_t batch_size);
    ~BatchDNN();
    void processResults();


    /// get a specific tensor from the batch
    /// the 'index' is stored in the tensor definition.
    TensorWrapper *tensor(size_t index) {return mTensors[index]; }

    /// access to the InferenceData
    InferenceData &inferenceData(size_t slot) { if (slot<mInferenceData.size()) return mInferenceData[slot];
        throw std::logic_error("Batch: invalid slot!");}

    /// extract data from the model and populate the examples for DNN inference
    bool fetchPredictors(Cell *cell, size_t slot);

    // access to the results for the examples (used to write classes from DNN to the batch)
    float *timeProbResult(size_t index) { return &mTimeProb[index * mNTopK]; }
    float *stateProbResult(size_t index) { return &mStateProb[index * mNTopK]; }
    state_t *stateResult(size_t index) { return &mStates[index * mNTopK]; }
private:
    void setupTensors();

    /// select from the topK classes (DNN result) the next state & time
    void selectClasses();
    size_t chooseProbabilisticIndex(float *values, size_t n);

    /// the data for the individual cells
    std::vector<InferenceData> mInferenceData;
    /// a vector of tensors associated with this batch of data
    std::vector<TensorWrapper*> mTensors;

    size_t mNTopK; ///< number of classes for each example
    /// store the topK classes from the DNN for target states
    std::vector<state_t> mStates;
    /// store the prob. for the topK classes from the DNN for target states
    std::vector<float> mStateProb;
    /// store the topK classes from the DNN for time
    std::vector<float> mTimeProb;

    friend class BatchManager;
};

#endif // BATCHDNN_H
