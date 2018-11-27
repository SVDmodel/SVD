#ifndef MODELRUNSTATE_H
#define MODELRUNSTATE_H

#include <string>
#include <vector>
#include <assert.h>


// global run state class for one individual state
class ModelRunState {
public:
    ModelRunState() : mState(Invalid) {  }
    enum State { Invalid=0, Creating, Created, ReadyToRun, Stopping, Running, Paused, Finished, Canceled, ErrorDuringSetup, Error };

    void set(State s) { if(mState!=s) { mState=s; update(); } }
    const State &state() const { return mState; }

    // operations
    ModelRunState &operator=(const State &s) { if (this->mState!=s) {this->mState = s; update(); } return *this; }
    bool operator==(const State &s) const { return this->mState == s; }
    bool operator!=(const State &s) const { return this->mState != s; }
    // return true if the current state is in the list of 'states' (e.g., state.in({Creating, Running})
    bool in(const std::vector<State> &states) const {  for (auto &s : states) if (mState==s) return true; return false; }

    std::string stateString(const ModelRunState &s) const;
    std::string stateString() const { return stateString(*this); }

    // combine two states
    State combine(const ModelRunState &s1, const ModelRunState &s2);
    void set(const ModelRunState &s1, const ModelRunState &s2) { set(combine(s1,s2)); }

    // semantic (content) functions
    bool shouldCancel() const { return mState==Stopping || mState==Error; }

private:
    // notify
    void update();
    State mState;
};

class RunState {
public:
    RunState();
    void clear();
    static RunState *instance() { assert(mInstance!=nullptr); return mInstance; }
    ModelRunState &modelState()  { return mModel; }
    ModelRunState &dnnState()  { return mDNN; }
    ModelRunState &state() { return mModel; }
    void set(ModelRunState state);
    // called by other states
    void update(ModelRunState *source);
    std::string asString() const;
    // global states
    bool cancel() const { return mCancel; }
    void setCancel(bool cnc) { mCancel=cnc; }

    void setError(std::string error_message, ModelRunState &state);

    // semantic queries
    bool isError() const { return mModel.in({ModelRunState::Error, ModelRunState::ErrorDuringSetup}); }
    bool isModelRunning() const { return mModel.in({ModelRunState::Creating, ModelRunState::Running, ModelRunState::Stopping}); }
    bool isModelFinished() const { return mModel.in({ModelRunState::Error, ModelRunState::ErrorDuringSetup, ModelRunState::Finished, ModelRunState::Canceled});}
    bool isModelPaused() const { return mModel.in({ ModelRunState::ReadyToRun, ModelRunState::Paused}); }
    bool isModelValid() const { return isModelRunning() || isModelFinished() || isModelPaused(); }
private:
    ModelRunState mModel;
    ModelRunState mDNN;
    ModelRunState mTotal;
    // commands
    bool mCancel; // should we cancel?
    std::string mErrorMessage;

    bool mInUpdate;
    static RunState *mInstance;
};

#endif // MODELRUNSTATE_H
