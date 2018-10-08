#include "inputtensoritem.h"

#include "strtools.h"
#include <list>
#include <algorithm>
#include <vector>


static std::map< std::string, InputTensorItem::DataContent> data_contents = {
    {"Invalid",       InputTensorItem::Invalid},
    {"Climate",       InputTensorItem::Climate},
    {"State",         InputTensorItem::State},
    {"ResidenceTime", InputTensorItem::ResidenceTime},
    {"Neighbors",     InputTensorItem::Neighbors},
    {"Site",          InputTensorItem::Site},
    {"Scalar",          InputTensorItem::Scalar},
    {"DistanceOutside", InputTensorItem::DistanceOutside}
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



InputTensorItem::InputTensorItem(std::string aname, std::string atype, size_t andim, size_t asizex, size_t asizey, std::string acontent)
{
    name=aname;
    type = datatypeFromString(atype);
    ndim = andim;
    sizeX = asizex;
    sizeY = asizey;
    content = contentFromString(acontent);
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
