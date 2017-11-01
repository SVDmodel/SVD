#ifndef CELL_H
#define CELL_H
#include "grid.h"
#include "states.h"

class Cell
{
public:
    Cell() : mStateId(-1), mResidenceTime(-1), mNextUpdateTime(-1), mState(nullptr), mNextStateId(-1) {}
    Cell(state_t state, restime_t res_time=0): mStateId(state), mResidenceTime(res_time), mNextUpdateTime(0), mNextStateId(-1) { setState(state); }

    // access
    bool isNull() const { return mStateId==-1; }
    /// the numeric ID of the state the cell is in
    state_t stateId() const { return mStateId; }
    /// get the State object the cell is in
    const State *state() const { return mState; }
    /// the time (number of years) the cell is already in the current state
    restime_t residenceTime() const { return mResidenceTime; }
    /// get the year for which the next update is scheduled
    int nextUpdate() const {return mNextUpdateTime; }

    bool needsUpdate() const;

    // actions
    /// check if update is scheduled and apply
    /// after update(), the cell is in the final state of the current year (31st of december)
    void update();
    void setState(state_t new_state);
    void setNextStateId(state_t new_state) { mNextStateId = new_state; }
    void setResidenceTime(restime_t res_time) { mResidenceTime = res_time; }
    void setNextUpdateTime(int next_year) { mNextUpdateTime = next_year; }
    void setInvalid() { mStateId=0; mResidenceTime=0; mState=nullptr; }

    /// get a vector with species shares (local, mid-range) for the current cell
    std::vector<double> neighborSpecies() const;
private:
    state_t mStateId; ///< the numeric ID of the state the cell is in
    restime_t mResidenceTime;
    int mNextUpdateTime; ///< the year (see Model::year()) when the next update of this cell is scheduled
    state_t mNextStateId; ///< the new state scheduled at mNextUpdateTime

    const State *mState;

    static std::vector<Point> mLocalNeighbors;
    static std::vector<Point> mMediumNeighbors;

};

#endif // CELL_H
