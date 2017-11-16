#ifndef CELL_H
#define CELL_H
#include "grid.h"
#include "states.h"

class Cell
{
public:
    Cell() : mStateId(-1), mResidenceTime(-1), mNextUpdateTime(-1), mState(nullptr), mNextStateId(-1), mExternalSeedType(-1) {}
    Cell(state_t state, restime_t res_time=0): mStateId(state), mResidenceTime(res_time), mNextUpdateTime(0), mNextStateId(-1), mExternalSeedType(-1) { setState(state); }

    // access
    /// isNull() returns true if the cell is an actively simulated cell
    bool isNull() const { return mStateId==-1; }
    /// the numeric ID of the state the cell is in
    state_t stateId() const { return mStateId; }
    /// get the State object the cell is in;
    /// do not use to check if the cell is simulated! (use isNull() instead)
    const State *state() const { return mState; }
    /// the time (number of years) the cell is already in the current state
    restime_t residenceTime() const { return mResidenceTime; }
    /// get the year for which the next update is scheduled
    int nextUpdate() const {return mNextUpdateTime; }

    /// returns true if the cell should be updated in the current year (i.e. if the DNN should be executed)
    bool needsUpdate() const;

    // actions
    /// check if update is scheduled (i.e. a state change should happen) and in case apply
    /// after update(), the cell is in the final state of the current year (31st of december)
    void update();
    void setState(state_t new_state);
    void setNextStateId(state_t new_state) { mNextStateId = new_state; }
    void setResidenceTime(restime_t res_time) { mResidenceTime = res_time; }
    void setNextUpdateTime(int next_year) { mNextUpdateTime = next_year; }
    void setInvalid() { mStateId=0; mResidenceTime=0; mState=nullptr; }

    bool hasExternalSeed() const { return mExternalSeedType>0 || (state()!=nullptr && !isNull()); }
    /// set external forest type:
    void setExternalSeedType(int new_type) { mExternalSeedType = new_type; }
    /// get external seed type
    int externalSeedType() const { return mExternalSeedType; }
    void setExternalState(state_t state);

    /// get a vector with species shares (local, mid-range) for the current cell
    std::vector<double> neighborSpecies() const;
private:
    state_t mStateId; ///< the numeric ID of the state the cell is in
    restime_t mResidenceTime;
    int mNextUpdateTime; ///< the year (see Model::year()) when the next update of this cell is scheduled
    state_t mNextStateId; ///< the new state scheduled at mNextUpdateTime
    int mExternalSeedType; ///< if the cell is outside of the project area, this type refers to the forest type

    const State *mState; ///< ptr to the State the cell currently is in



    static std::vector<Point> mLocalNeighbors;
    static std::vector<Point> mMediumNeighbors;

};

#endif // CELL_H
