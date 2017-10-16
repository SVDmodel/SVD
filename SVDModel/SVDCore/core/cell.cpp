#include "cell.h"
#include "model.h"

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
        if (mNextState != state()) {
            setState( mNextState );
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
