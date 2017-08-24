#ifndef STATES_H
#define STATES_H

#include <string>
#include <vector>

class State {
public:
    State(int id, std::string composition, int structure, int function);
    int id() const { return mId; }
    const std::string &compositionString() const { return mComposition; }
    int function() const { return mFunction; }
    int structure() const {return mStructure; }
private:
    int mId;
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
