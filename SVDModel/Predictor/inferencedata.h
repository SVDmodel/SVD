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
#ifndef INFERENCEDATA_H
#define INFERENCEDATA_H
#include "cell.h"
#include "states.h"
#include "batchmanager.h"
#include "environmentcell.h"
class BatchDNN; // forward
class Batch; // forward

class InferenceData
{
public:
    InferenceData(): mOldState(-1), mNextState(-1), mNextTime(-1), mBatch(nullptr), mSlot(std::numeric_limits<size_t>::max()) {}

    /// fills an item by pulling all the required data from the model
    void fetchData(Cell *cell, BatchDNN *batch, size_t slot);

    /// set the result of the DNN
    void setResult(state_t state, restime_t time);
    state_t nextState() const { return mNextState; }
    state_t state() const { return mOldState; }
    /// the absolute time (year) the next evaluation / state change will happen.
    restime_t nextTime() const { return mNextTime; }
    /// write the result back to the cell in the model
    void writeResult();

    // access
    int cellIndex() const { return mIndex; }
    const EnvironmentCell &environmentCell() const;
    const Cell &cell() const;

    /// get a human readable string from the data in the tensors for the example in 'slot'
    std::string dumpTensorData();
private:
    state_t mOldState;
    state_t mNextState;
    restime_t mNextTime;
    restime_t mResidenceTime;
    int mIndex; ///< index of the cell in the landscape grid
    BatchDNN *mBatch; ///< link to the batch (that contains the actual Tensors)
    size_t mSlot; ///< slot within the batch for this cell
};

#endif // INFERENCEDATA_H
