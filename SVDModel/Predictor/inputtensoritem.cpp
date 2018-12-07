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
#include "inputtensoritem.h"

#include "strtools.h"
#include <list>
#include <algorithm>
#include <vector>

#include "fetchdata.h"

static std::map< std::string, InputTensorItem::DataContent> data_contents = {
    {"Invalid",       InputTensorItem::Invalid},
    {"Climate",       InputTensorItem::Climate},
    {"State",         InputTensorItem::State},
    {"ResidenceTime", InputTensorItem::ResidenceTime},
    {"Neighbors",     InputTensorItem::Neighbors},
    {"Var",          InputTensorItem::Variable},
    {"Scalar",          InputTensorItem::Scalar},
    {"DistanceOutside", InputTensorItem::DistanceOutside},
    {"SiteNPKA",         InputTensorItem::SiteNPKA},
    {"Function",         InputTensorItem::Function }
};
static std::map< std::string, InputTensorItem::DataType> data_types = {
    {"Invalid", InputTensorItem::DT_INVALID},
    {"float",   InputTensorItem::DT_FLOAT},
    {"int16",   InputTensorItem::DT_INT16},
    {"int64",   InputTensorItem::DT_INT64},
    {"uint16",  InputTensorItem::DT_UINT16},
    {"float16", InputTensorItem::DT_BFLOAT16},
    {"bool",    InputTensorItem::DT_BOOL}
};

template <typename T>
std::string keys_to_string(const std::map<std::string, T> &mp) {
    std::vector<std::string> s;
    for (auto it : mp)
        s.push_back(it.first);
    return join(s, ",");
}



InputTensorItem::~InputTensorItem()
{
    if (mFetch) delete mFetch;
}

InputTensorItem::InputTensorItem(std::string aname, std::string atype, size_t andim, size_t asizex, size_t asizey, std::string acontent)
{
    name=aname;
    type = datatypeFromString(atype);
    ndim = andim;
    sizeX = asizex;
    sizeY = asizey;
    content = contentFromString(acontent);
    mFetch = nullptr;
}

InputTensorItem::DataContent InputTensorItem::contentFromString(std::string name)
{
    if (data_contents.find(name) == data_contents.end())
        throw std::logic_error("'" + name + "' is not a valid code for data content (definition of input tensors - check the log)!");
    return data_contents[name];

}

InputTensorItem::DataType InputTensorItem::datatypeFromString(std::string name)
{
    if (data_types.find(name) == data_types.end())
        throw std::logic_error("'" + name + "' is not a valid code for a data type (definition of input tensors - check the log)!");
    return data_types[name];

}

std::string InputTensorItem::contentString(InputTensorItem::DataContent content)
{
    auto it = std::find_if(data_contents.begin(), data_contents.end(), [=](const auto &value) { return value.second == content; });
    if (it!=data_contents.end())
        return it->first;
    else
        return "invalid!";
}

std::string InputTensorItem::datatypeString(InputTensorItem::DataType dtype)
{
    auto it = std::find_if(data_types.begin(), data_types.end(), [=](const auto &value) { return value.second == dtype; });
    if (it!=data_types.end())
        return it->first;
    else
        return "invalid!";

}

std::string InputTensorItem::allDataTypeStrings()
{
    return keys_to_string(data_types);
}

std::string InputTensorItem::allContentStrings()
{
    return keys_to_string(data_contents);
}
