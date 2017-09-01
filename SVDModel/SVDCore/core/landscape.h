#ifndef LANDSCAPE_H
#define LANDSCAPE_H

#include <cassert>

#include "grid.h"
#include "cell.h"
#include "environmentcell.h"


class Landscape
{
public:
    Landscape();
    void setup();


    // access
    // access single cells
    Cell &cell(int index) { assert((*mCurrentGrid).isIndexValid(index));
                            return (*mCurrentGrid)[index]; }
    EnvironmentCell &environmentCell(int index) { assert(mEnvironmentGrid.isIndexValid(index));
                                                assert(mEnvironmentGrid[index] != nullptr);
                                                return *mEnvironmentGrid[index]; }

    // access whole grids
    /// get the grid of Cells. Cells outside the project area are marked
    /// by cells where isNull() is true.
    Grid<Cell> &currentGrid() const { return *mCurrentGrid; }
    Grid<Cell> &futureGrid() const { return *mFutureGrid; }
    /// environment-grid: pointer to EnvironmentCell, nullptr if invalid.
    Grid<EnvironmentCell*> &environment()  { return mEnvironmentGrid; }

    void switchStates();

private:
    void setupInitialState();
    Grid<Cell> mGridA;
    Grid<Cell> mGridB;
    Grid<Cell> *mCurrentGrid;
    Grid<Cell> *mFutureGrid;

    Grid<EnvironmentCell*> mEnvironmentGrid;
    std::vector<EnvironmentCell> mEnvironmentCells;


};

#endif // LANDSCAPE_H
