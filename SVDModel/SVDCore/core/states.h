#ifndef STATES_H
#define STATES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>
#include "strtools.h"

/// state_t:
typedef short int state_t; // 16bit
typedef short int restime_t; // 16bit

class Module; // forward

class State {
public:
    enum StateType { Forest=0, Grass=1, None=99 };
    State(state_t id, std::string composition, int structure, int function, StateType type);
    StateType type() const { return mType; }
    state_t id() const { return mId; }
    const std::string &compositionString() const { return mComposition; }
    int function() const { return mFunction; }
    int structure() const {return mStructure; }
    std::string asString() const;

    /// set the module handling this state
    void setModule(Module *module) { mModule = module; }
    /// get the handling module for the state
    Module *module() const { return mModule; }

    const std::vector<double> &speciesShares() const { return mSpeciesShare; }

    // properties
    static const std::vector<std::string> &valueNames() { return mValueNames; }
    static int valueIndex(const std::string &name) { return indexOf(mValueNames, name); }
    bool hasValue(const std::string &name) const { return indexOf(mValueNames, name)>=0; }
    double value(const std::string &name) const { return value(static_cast<size_t>(valueIndex(name))); }
    double value(const size_t index) const { assert(index>=0); size_t i=static_cast<size_t>(index); return i<mValues.size() ? mValues[i] : 0.;}
    void setValue(const std::string &name, double value);
    void setValue(const int index, double value);
private:
    state_t mId;
    std::string mComposition;
    int mStructure;
    int mFunction;
    StateType mType;
    std::vector<double> mSpeciesShare;

    Module *mModule;

    /// store for extra state specific values used by modules
    std::vector<double> mValues;
    static std::vector<std::string> mValueNames;
};

class States
{
public:
    States();
    void setup();

    /// load properties from a text file (stateId is the key)
    bool loadProperties(const std::string &filename);

    // members
    bool isValid(state_t state) const { return mStateSet.find(state) != mStateSet.end(); }
    const State &randomState() const;
    const std::vector<State> &states() { return mStates; }
    const State &stateByIndex(size_t index) const { return mStates[index]; }
    const State &stateById(state_t id);

    // handlers
    bool registerHandler(Module *module, State::StateType state_type);
    /// update the handlers of all states
    void updateStateHandlers();

private:
    std::vector<State> mStates;
    std::unordered_map<state_t, size_t> mStateSet;
    std::map<State::StateType, Module *> mHandlers;

};

#endif // STATES_H
