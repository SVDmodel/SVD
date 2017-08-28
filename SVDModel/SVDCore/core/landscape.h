#ifndef LANDSCAPE_H
#define LANDSCAPE_H

#include "grid.h"
#include "cell.h"

class LandscapeState {
public:
private:
    std::vector<Cell> mCells;
    Grid<Cell*> mGrid;
friend class Landscape;
};

class Landscape
{
public:
    Landscape();
    void setup();
    LandscapeState &currentState() const { return *mCurrentState; }
    LandscapeState &futureState() const { return *mFutureState; }
private:
    void switchStates();
    LandscapeState mStateA;
    LandscapeState mStateB;
    LandscapeState *mCurrentState;
    LandscapeState *mFutureState;

};

#endif // LANDSCAPE_H
