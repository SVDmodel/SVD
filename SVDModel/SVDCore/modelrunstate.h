#ifndef MODELRUNSTATE_H
#define MODELRUNSTATE_H

#include <string>
#include <vector>

// global run state class
class ModelRunState {
public:
    ModelRunState() : mState(Invalid) {}
    enum State { Invalid=0, Creating, ReadyToRun, Stopping, Running, Paused, Finished, Canceled, ErrorDuringSetup, Error };

    void set(State s) { mState=s; }
    State state() const { return mState; }

    // operations
    ModelRunState &operator=(const State &s) { this->mState = s; return *this; }
    bool operator==(const State &s) const { return this->mState == s; }
    // return true if the current state is in the list of 'states' (e.g., state.in({Creating, Running})
    bool in(const std::vector<State> &states) const {  for (auto &s : states) if (mState==s) return true; return false; }

    std::string stateString(ModelRunState &s);
    std::string stateString() { return stateString(*this); }

    // combine two states
    State combine(const ModelRunState &s1, const ModelRunState &s2);
    void set(const ModelRunState &s1, const ModelRunState &s2) { set(combine(s1,s2)); }

    // semantic (content) functions
    bool shouldCancel() const { return mState==Stopping || mState==Error; }

private:
    State mState;
};



#endif // MODELRUNSTATE_H
