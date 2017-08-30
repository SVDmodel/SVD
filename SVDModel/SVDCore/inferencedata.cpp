#include "inferencedata.h"
#include "model.h"

InferenceData::InferenceData(Cell *cell)
{
    mOldState = cell->state();
    mNextState = 0;
    mNextTime = 0;
    mIndex = cell - Model::instance()->landscape()->currentGrid().begin();
}

void InferenceData::writeResult()
{
    // write back:
    Cell &cell = Model::instance()->landscape()->futureGrid()[size_t(mIndex)];
    cell.setState(mNextState);
    cell.setNextUpdateTime(mNextTime);
    cell.setResidenceTime(0);

}
