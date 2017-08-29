#include "cell.h"
#include "model.h"

bool Cell::needsUpdate() const
{
    if (Model::instance()->year() >= mNextUpdateTime)
        return true;
    return false;
}
