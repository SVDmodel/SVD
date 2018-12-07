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
#ifndef INPUTTENSORITEM_H
#define INPUTTENSORITEM_H

#include <string>
#include <map>

// #include "fetchdata.h"
class FetchData; // forward
struct InputTensorItem {
    ~InputTensorItem();
    enum DataContent {
        Invalid = 0,
        Climate = 1,
        State = 2,
        ResidenceTime = 3,
        Neighbors = 4,
        Variable = 5,
        Scalar = 6,
        DistanceOutside = 7,
        SiteNPKA = 8, // old static NPKA site
        Function = 9
    };

    /// supported data types (values copied from tensorflow types.pb.h)
    enum DataType {
        DT_INVALID = 0,
        DT_FLOAT = 1,
        DT_INT16 = 5,
        DT_INT64 = 9,
        DT_BOOL = 10,
        DT_UINT16 = 17,
        DT_BFLOAT16 = 14
    };
    InputTensorItem(std::string aname, DataType atype, size_t andim, size_t asizex, size_t asizey, DataContent acontent):
        name(aname), type(atype), ndim(andim), sizeX(asizex), sizeY(asizey), content(acontent), mFetch(nullptr){}
    InputTensorItem(std::string aname, std::string atype, size_t andim, size_t asizex, size_t asizey, std::string acontent);
    std::string name; ///< the name of the tensor within the DNN
    DataType type; ///< data type enum
    size_t ndim; ///< the number of dimensions (the batch dimension is added automatically)
    size_t sizeX; ///< number of data elements in the first dimension
    size_t sizeY; ///< number of elements in the second dimension (for 2-dimensional input data)
    DataContent content; ///< the (semantic) type of data
    size_t index; ///< the index in the list of tensors

    // data fetch machina
    FetchData *mFetch;

    // helpers
    static DataContent contentFromString(std::string name);
    static DataType datatypeFromString(std::string name);
    static std::string contentString(DataContent content);
    static std::string datatypeString(DataType dtype);
    static std::string allDataTypeStrings();
    static std::string allContentStrings();
};


#endif // INPUTTENSORITEM_H
