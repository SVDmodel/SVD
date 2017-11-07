#ifndef STATES_H
#define STATES_H

#include <string>
#include <vector>
#include <unordered_map>

/// state_t:
typedef short int state_t;
typedef short int restime_t;

class State {
public:
    State(state_t id, std::string composition, int structure, int function);
    state_t id() const { return mId; }
    const std::string &compositionString() const { return mComposition; }
    int function() const { return mFunction; }
    int structure() const {return mStructure; }
    std::string asString() const;
    const std::vector<double> &speciesShares() const { return mSpeciesShare; }
private:
    state_t mId;
    std::string mComposition;
    int mStructure;
    int mFunction;
    std::vector<double> mSpeciesShare;
};

class States
{
public:
    States();
    void setup();

    // members
    bool isValid(state_t state) const { return mStateSet.find(state) != mStateSet.end(); }
    const State &randomState() const;
    const std::vector<State> &states() { return mStates; }
    const State &stateByIndex(size_t index) const { return mStates[index]; }
    const State &stateById(int id);

private:
    std::vector<State> mStates;
    std::unordered_map<state_t, size_t> mStateSet;

};

#endif // STATES_H
