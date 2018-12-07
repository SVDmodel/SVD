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
#ifndef STATECHANGEOUT_H
#define STATECHANGEOUT_H

#include "output.h"
#include "../../Predictor/inferencedata.h"
#include "expression.h"

class StateChangeOut : public Output
{
public:
    StateChangeOut();
    void setup();
    void execute();
    //void executeBatch(const TensorWrap2d<int32> &state_index, const TensorWrap2d<float> &scores, const TensorWrap2d<float> &time, int n);
    bool shouldWriteOutput(const InferenceData &id);

    void writeLine(std::string content);
private:
    int mInterval;
    int mCellId;
    Expression mFilter;
};

#endif // STATECHANGEOUT_H
