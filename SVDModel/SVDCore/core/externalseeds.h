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
#ifndef EXTERNALSEEDS_H
#define EXTERNALSEEDS_H

#include <map>
#include <vector>
class FileReader; // forward
template<typename T> class Grid; // forward
class ExternalSeeds
{
public:
    ExternalSeeds();
    void setup();

    const std::vector<double> &speciesShares(int type_id) const;
private:
    bool mTableMode;
    int setupFromSpeciesShares(FileReader &rdr, Grid<int> &eseed);
    int setupFromStates(FileReader &rdr, Grid<int> &eseed);
    std::map<int, std::vector<double> > mExternalSeedTypes;
};

#endif // EXTERNALSEEDS_H
