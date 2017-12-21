#ifndef STATECHANGEOUT_H
#define STATECHANGEOUT_H

#include "output.h"
#include "../../Predictor/inferencedata.h"
#include "expression.h"
#include <fstream>
class StateChangeOut : public Output
{
public:
    StateChangeOut();
    void setup();
    void execute();
    //void executeBatch(const TensorWrap2d<int32> &state_index, const TensorWrap2d<float> &scores, const TensorWrap2d<float> &time, int n);
    bool shouldWriteOutput(const InferenceData &id);

    void writeLine(std::string content);
private:
    int mInterval;
    int mCellId;
    Expression mFilter;
    std::string mOutputFile;
};

#endif // STATECHANGEOUT_H
