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
#ifndef CLIMATE_H
#define CLIMATE_H

#include <vector>
#include <unordered_map>
#include <set>


class Climate
{
public:
    Climate();
    void setup();

    // access
    /// retrieve a list of climate series, starting from 'start_year' (first year: 1, ...)
    /// and with the given length ('series_length').
    std::vector< const std::vector<float>* > series(int start_year, size_t series_length, int climateId) const;
    const std::vector<float> &singleSeries(const int year, const int climateId) const { return mData.at(year).at(climateId); }
    bool hasSeries(const int year, const int climateId) const { auto y=mData.find(year); if(y==mData.end()) return false;
                                                                auto s= y->second.find(climateId); if (s==(*y).second.end()) return false;
                                                              return true;}
private:
    size_t mNColumns; ///< the number of data elements per year+id
    /// the main container for climate data
    /// the structure is: "year" -> "climateId" + data
    std::unordered_map< int, std::unordered_map<int, std::vector<float> > > mData;
    std::set<int> mAllYears;
    std::set<int> mAllIds;
    /// indices of years to use
    std::vector<int> mSequence;
};

#endif // CLIMATE_H
