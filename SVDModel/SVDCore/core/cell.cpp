#include "cell.h"
#include "model.h"

// 37 values, roughly a circle with 7px diameter
std::vector<Point> Cell::mMediumNeighbors = {
              {-1,-3}, {0,-3}, {1,-3},
        {-2,-2}, {-1,-2},{0,-2},{1,-2},{2,-2},
    {-3,-1}, {-2,-1},{-1,-1},{0,-1},{1,-1},{2,-1},{3,-1},
    {-3,0}, {-2,0}, {-1,0},          {1,0}, {2,0}, {3,0},
    {-3,1}, {-2,1} ,{-1,1} ,{0,1} ,{1,1} ,{2,1} ,{3,1},
           {-2,2}, {-1,2},{0,2},{1,2},{2,2},
               {-1,3}, {0,3}, {1,3},

};

// local neighbors: the moore neighborhood (8 values)
std::vector<Point> Cell::mLocalNeighbors = {
    {-1,-1}, { 0,-1}, { 1,-1},
    {-1, 0},          { 1, 0},
    {-1, 1}, { 0, 1}, { 1, 1}
};




bool Cell::needsUpdate() const
{
    if (Model::instance()->year() >= mNextUpdateTime)
        return true;
    return false;
}

void Cell::update()
{
    // is called at the end of the year: the state changes
    // already now so that we will have the correct state at the
    // start of the next year
    int year = Model::instance()->year();
    if (year+1 >= mNextUpdateTime) {
        // change the state of the current cell
        if (mNextStateId != stateId()) {
            setState( mNextStateId );
            setResidenceTime( 0 );
        } else {
            // the state is not changed;
            // nonetheless, the cell will be re-evaluated in the next year
        }
    } else {
        // no update. The residence time changes.
        mResidenceTime++;
    }

}

void Cell::setState(state_t new_state)
{
    mStateId = new_state;
    if (new_state<0)
        mState=nullptr;
    else
        mState = &Model::instance()->states()->stateById(new_state);
}

std::vector<double> Cell::neighborSpecies() const
{
    auto &grid =  Model::instance()->landscape()->grid();
    size_t n_species = Model::instance()->species().size();
    Point center = grid.indexOf(this);
    std::vector<double> result(n_species*2, 0.);
    // local neighbors
    double n_local = 0.;
    for (const auto &p : mLocalNeighbors) {
        if (grid.isIndexValid(center + p)) {
            Cell &cell = grid.valueAtIndex(center + p);
            if (cell.state()) {
                auto shares = cell.state()->speciesShares();
                for (int i=0; i<n_species;++i)
                    result[i*2] += shares[i];
                ++n_local;
            }
        }
    }
    if (n_local>0.)
        for (int i=0; i<n_species;++i)
            result[i*2] /= n_local;

    // mid-range neighbors
    double n_mid = 0.;
    for (const auto &p : mLocalNeighbors) {
        if (grid.isIndexValid(center + p)) {
            Cell &cell = grid.valueAtIndex(center + p);
            if (cell.state()) {
                auto shares = cell.state()->speciesShares();
                for (int i=0; i<n_species;++i)
                    result[i*2+1] += shares[i];
                ++n_mid;
            }
        }
    }
    if (n_mid>0.)
        for (int i=0; i<n_species;++i)
            result[i*2+1] /= n_mid;

    return result;
}
