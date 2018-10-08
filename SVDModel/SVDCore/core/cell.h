#ifndef CELL_H
#define CELL_H
#include "grid.h"
#include "states.h"

class EnvironmentCell; // forward
class Cell
{
public:
    Cell() : mStateId(-1), mResidenceTime(-1), mNextUpdateTime(-1), mNextStateId(-1),  mExternalSeedType(-1), mIsUpdated(false), mState(nullptr), mEnvCell(nullptr) {}
    Cell(state_t state, restime_t res_time=0): mStateId(state), mResidenceTime(res_time), mNextUpdateTime(0), mNextStateId(-1), mExternalSeedType(-1), mIsUpdated(false), mEnvCell(nullptr) { setState(state); }
    /// establish the link to the environment cell
    void setEnvironmentCell(const EnvironmentCell *ec) { mEnvCell = ec; }

    // access
    /// isNull() returns true if the cell is not an actively simulated cell
    bool isNull() const { return mStateId==-1; }
    /// the numeric ID of the state the cell is in
    state_t stateId() const { return mStateId; }
    /// get the State object the cell is in;
    /// do not use to check if the cell is part of the simulated landscape! (use isNull() instead)
    const State *state() const { return mState; }
    /// the time (number of years) the cell is already in the current state
    restime_t residenceTime() const { return mResidenceTime; }
    /// get the year for which the next update is scheduled
    int nextUpdate() const {return mNextUpdateTime; }
    const EnvironmentCell *environment() const { return mEnvCell; }

    /// returns true if the cell should be updated in the current year (i.e. if the DNN should be executed)
    bool needsUpdate() const;

    // actions
    /// check if update is scheduled (i.e. a state change should happen) and in case apply the update;
    /// after update(), the cell is in the final state of the current year ("31st of december")
    void update();
    void setState(state_t new_state);
    void setResidenceTime(restime_t res_time) { mResidenceTime = res_time; }

    void setNextStateId(state_t new_state) { if(!mIsUpdated) mNextStateId = new_state; }
    void setNextUpdateTime(int next_year) { if(!mIsUpdated) mNextUpdateTime = next_year; }
    /// sets a new state immediately (later updates from DNN are blocked)
    void setNewState(state_t new_state);
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
    void dumpDebugData();
    state_t mStateId; ///< the numeric ID of the state the cell is in
    restime_t mResidenceTime;
    int mNextUpdateTime; ///< the year (see Model::year()) when the next update of this cell is scheduled
    state_t mNextStateId; ///< the new state scheduled at mNextUpdateTime
    int mExternalSeedType; ///< if the cell is outside of the project area, this type refers to the forest type
    bool mIsUpdated; ///< flag indicating that the state is already updated (e.g. by management)

    const State *mState; ///< ptr to the State the cell currently is in
    const EnvironmentCell *mEnvCell; ///< ptr to the environment



    static std::vector<Point> mLocalNeighbors;
    static std::vector<Point> mMediumNeighbors;

};

#endif // CELL_H
