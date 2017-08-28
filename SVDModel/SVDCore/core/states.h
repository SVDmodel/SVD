#ifndef STATES_H
#define STATES_H

#include <string>
#include <vector>

typedef short int statetype;

class State {
public:
    State(statetype id, std::string composition, int structure, int function);
    statetype id() const { return mId; }
    const std::string &compositionString() const { return mComposition; }
    int function() const { return mFunction; }
    int structure() const {return mStructure; }
private:
    statetype mId;
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
    const std::vector<State> &states() { return mStates; }

private:
    std::vector<State> mStates;

};

#endif // STATES_H
