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

    /// a list of all climate ids (regions) that are present in the current landscape
    const std::map<int, int> &climateIds() { return mClimateIds; }

private:
    void setupInitialState();
    Grid<Cell> mGrid;

    Grid<EnvironmentCell*> mEnvironmentGrid;
    std::vector<EnvironmentCell> mEnvironmentCells;

    std::map<int, int> mClimateIds;
};

#endif // LANDSCAPE_H
