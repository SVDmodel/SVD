#ifndef TRANSITIONMATRIX_H
#define TRANSITIONMATRIX_H
#include <map>
#include <vector>

#include "states.h"

class TransitionMatrix
{
public:
    TransitionMatrix();
    bool load(const std::string &filename);

    // access

    /// choose a next state from the transition matrix
    state_t transition(state_t stateId, int key=0);
    /// check if the state stateId has stored transition values
    bool isValid(state_t stateId, int key=0) { return mTM.find({stateId, key}) != mTM.end(); }
private:
    /// storage for transition matrix: key: state + numerical key, content: list of target states + probabilties
    std::map< std::pair<state_t, int>,
              std::vector< std::pair< state_t, double > > > mTM;
};

#endif // TRANSITIONMATRIX_H
