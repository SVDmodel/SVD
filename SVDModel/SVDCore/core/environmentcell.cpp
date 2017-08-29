#include "environmentcell.h"

std::vector<std::string> EnvironmentCell::mVariables = {};



void EnvironmentCell::setValue(const int var_idx, double new_value)
{
    if (var_idx<0 || var_idx>=mVariables.size())
        throw std::logic_error("EnvironmentCell::setValue: invalid index.");
    if (mValues.size()<mVariables.size())
        mValues.resize(mVariables.size());
    mValues[var_idx] = new_value;
}
