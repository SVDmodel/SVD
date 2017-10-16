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
    Cell &cell(int index) { assert(mGrid.isIndexValid(index));
                            return mGrid[index]; }
    EnvironmentCell &environmentCell(int index) { assert(mEnvironmentGrid.isIndexValid(index));
                                                assert(mEnvironmentGrid[index] != nullptr);
                                                return *mEnvironmentGrid[index]; }

    // access whole grids
    /// get the grid of Cells. Cells outside the project area are marked
    /// by cells where isNull() is true.
    Grid<Cell> &grid() { return mGrid; }
    /// environment-grid: pointer to EnvironmentCell, nullptr if invalid.
    Grid<EnvironmentCell*> &environment()  { return mEnvironmentGrid; }


private:
    void setupInitialState();
    Grid<Cell> mGrid;

    Grid<EnvironmentCell*> mEnvironmentGrid;
    std::vector<EnvironmentCell> mEnvironmentCells;


};

#endif // LANDSCAPE_H
