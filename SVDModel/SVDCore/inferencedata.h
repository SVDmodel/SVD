#ifndef INFERENCEDATA_H
#define INFERENCEDATA_H
#include "cell.h"
#include "states.h"

class InferenceData
{
public:
    InferenceData(Cell *cell);
    void setResult(state_t state, restime_t time) { mNextState=state; mNextTime=time; }

    void writeResult();
private:
    state_t mOldState;
    state_t mNextState;
    restime_t mNextTime;
    size_t mIndex;
};

#endif // INFERENCEDATA_H
