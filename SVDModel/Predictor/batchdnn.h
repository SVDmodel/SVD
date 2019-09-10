/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#ifndef BATCHDNN_H
#define BATCHDNN_H

#include "batch.h"
#include "inferencedata.h"

class StateChangeOut; // forward

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
    float *timeProbResult(size_t index) { return &mTimeProb[index * mNTimeClasses]; }
    float *stateProbResult(size_t index) { return &mStateProb[index * mNTopK]; }
    state_t *stateResult(size_t index) { return &mStates[index * mNTopK]; }
private:
    void setupTensors();

    /// select from the topK classes (DNN result) the next state & time
    void selectClasses();
    size_t chooseProbabilisticIndex(float *values, size_t n);

    // state change output specific
    /// link to detailed output
    static StateChangeOut *mSCOut;
    std::string stateChangeOutput(size_t index);

    /// the data for the individual cells
    std::vector<InferenceData> mInferenceData;
    /// a vector of tensors associated with this batch of data
    std::vector<TensorWrapper*> mTensors;

    size_t mNTopK; ///< number of classes for each example
    size_t mNTimeClasses; ///< number of time classes for each example
    bool mAllowStateChangeAtMaxTime; ///< if true, selecting the maximum number of years forces the state to stay the same
    /// store the topK classes from the DNN for target states
    std::vector<state_t> mStates;
    /// store the prob. for the topK classes from the DNN for target states
    std::vector<float> mStateProb;
    /// store the topK classes from the DNN for time
    std::vector<float> mTimeProb;

    friend class BatchManager;
};

#endif // BATCHDNN_H
