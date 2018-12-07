/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
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
