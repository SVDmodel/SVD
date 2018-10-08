#include "batchdnn.h"

#include "tensorhelper.h"

#include "model.h"

#include "randomgen.h"

#include "dnn.h"

BatchDNN::BatchDNN(size_t batch_size) : Batch(batch_size)
{
    mType = DNN;

    mInferenceData.resize(mBatchSize);
    // reserve memory for the topK classes for target states and residence time

    mNTopK = Model::instance()->settings().valueUInt("dnn.topKNClasses", 10);
    mStates.resize(mBatchSize * mNTopK);
    mStateProb.resize(mBatchSize * mNTopK);
    mTimeProb.resize(mBatchSize * mNTopK);

    setupTensors();

}

BatchDNN::~BatchDNN()
{
    // free the memory of the tensors...
    if (spdlog::get("dnn"))
        spdlog::get("dnn")->trace("Destructor of batch, free tensors");
    for (auto p : mTensors) {
        delete p;
    }

}

void BatchDNN::processResults()
{

    auto lg = spdlog::get("main");

    lg->debug("Model: received package {} [{}](from DNN). Processing data.", packageId(), static_cast<void*>(this));

    // choose from the topK classes
    selectClasses();

    for (size_t i=0;i<usedSlots();++i) {
        inferenceData(i).writeResult();
    }

}

bool BatchDNN::fetchPredictors(Cell *cell, size_t slot)
{
    inferenceData(slot).fetchData(cell, this, slot);
    return true;
}

void BatchDNN::setupTensors()
{
    DNN::instance()->setupBatch(this, mTensors);
}

// choose randomly a value in *values (length=n), return the index.
// if 'skip_index' != -1, then this index is not allowed (and the skipped)
size_t BatchDNN::chooseProbabilisticIndex(float *values, size_t n)
{

    // calculate the sum of probs
    double p_sum = 0.;
    for (size_t i=0;i<n;++i)
        p_sum+= static_cast<double>(values[i]);

    double p = nrandom(0., p_sum);

    p_sum = 0.;
    for (size_t i=0;i<n;++i, ++values) {
        p_sum += static_cast<double>(values[i]);
        if (p < p_sum)
            return i;
    }
    return n-1;
}


void BatchDNN::selectClasses()
{
    // Now select for each example the result of the prediction
    // choose randomly from the result
    for (size_t i=0; i<usedSlots(); ++i) {
        InferenceData &id = inferenceData(i);
        // residence time: at least one year
        restime_t rt = static_cast<restime_t>( chooseProbabilisticIndex(timeProbResult(i), mNTopK )) + 1;
        //restime_t rt = static_cast<restime_t>( chooseProbabilisticIndex(out_time.example(i), static_cast<int>(out_time.n())) ) + 1;
        if (rt == static_cast<restime_t>(mNTopK)) {
            // the state will be the same for the next period (no change)
            id.setResult(id.state(), rt);
        } else {
            // select the next state probalistically
            // the next state is not allowed to stay the same -> set probability to 0
            for (size_t j=0;j<mNTopK;++j) {
                if (stateResult(i)[j] == id.state()) {
                    stateProbResult(i)[j] = 0.f;
                    break;
                }
            }

            size_t index = chooseProbabilisticIndex(stateProbResult(i), mNTopK);
            state_t stateId = stateResult(i)[index];
            //size_t state_index = static_cast<size_t>(indices_flat.example(i)[index]);
            //state_t stateId = Model::instance()->states()->stateByIndex( state_index ).id();
            if (stateId == 0 || rt == 0) {
                spdlog::get("main")->error("bad data in batch {} with {} used slots. item {}: update-time: {}, update-state: {} (set state to 1)", packageId(), usedSlots(), i, inferenceData(i).nextTime(), inferenceData(i).nextState());
                if (stateId==0)
                    stateId = 1;
                if (rt == 0)
                    rt = 1;
            }
            id.setResult(stateId, rt);
        }
    }

}
