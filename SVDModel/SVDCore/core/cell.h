#ifndef CELL_H
#define CELL_H
#include "states.h"

class Cell
{
public:
    Cell() : mState(-1), mResidenceTime(-1), mNextUpdateTime(-1) {}
    Cell(state_t state, restime_t res_time=0): mState(state), mResidenceTime(res_time), mNextUpdateTime(0) {}

    // access
    bool isNull() const { return mState==-1; }
    /// the numeric ID of the state the cell is in
    state_t state() const { return mState; }
    /// the time (number of years) the cell is already in the current state
    restime_t residenceTime() const { return mResidenceTime; }
    /// get the year for which the next update is scheduled
    int nextUpdate() const {return mNextUpdateTime; }

    bool needsUpdate() const;

    // actions
    /// check if update is scheduled and apply
    /// after update(), the cell is in the final state of the current year (31st of december)
    void update();
    void setState(state_t new_state) { mState = new_state; }
    void setNextState(state_t new_state) { mNextState = new_state; }
    void setResidenceTime(restime_t res_time) { mResidenceTime = res_time; }
    void setNextUpdateTime(int next_year) { mNextUpdateTime = next_year; }
    void setInvalid() { mState=0; mResidenceTime=0; }
private:
    state_t mState; ///< the numeric ID of the state the cell is in
    restime_t mResidenceTime;
    int mNextUpdateTime; ///< the year (see Model::year()) when the next update of this cell is scheduled
    state_t mNextState; ///< the new state scheduled at mNextUpdateTime
};

#endif // CELL_H
