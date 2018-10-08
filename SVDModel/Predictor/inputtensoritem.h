#ifndef INPUTTENSORITEM_H
#define INPUTTENSORITEM_H

#include <string>
#include <map>


struct InputTensorItem {
    enum DataContent {
        Invalid = 0,
        Climate = 1,
        State = 2,
        ResidenceTime = 3,
        Neighbors = 4,
        Site = 5,
        Scalar = 6,
        DistanceOutside = 7
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
        name(aname), type(atype), ndim(andim), sizeX(asizex), sizeY(asizey), content(acontent){}
    InputTensorItem(std::string aname, std::string atype, size_t andim, size_t asizex, size_t asizey, std::string acontent);
    std::string name; ///< the name of the tensor within the DNN
    DataType type; ///< data type enum
    size_t ndim; ///< the number of dimensions (the batch dimension is added automatically
    size_t sizeX; ///< number of data elements in the first dimension
    size_t sizeY; ///< number of elements in the second dimension (for 2-dimensional input data)
    DataContent content; ///< the (semantic) type of data
    size_t index; ///< the index in the list of tensors

    // helpers
    static DataContent contentFromString(std::string name);
    static DataType datatypeFromString(std::string name);
    static std::string contentString(DataContent content);
    static std::string datatypeString(DataType dtype);
    static std::string allDataTypeStrings();
    static std::string allContentStrings();
};


#endif // INPUTTENSORITEM_H
