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
#include "environmentcell.h"

std::vector<std::string> EnvironmentCell::mVariables = {};



void EnvironmentCell::setValue(const int var_idx, double new_value)
{
    if (var_idx<0 || static_cast<size_t>(var_idx)>=mVariables.size())
        throw std::logic_error("EnvironmentCell::setValue: invalid index.");
    if (mValues.size()<mVariables.size())
        mValues.resize(mVariables.size());
    mValues[static_cast<size_t>(var_idx)] = new_value;
}
