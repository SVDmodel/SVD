#ifndef TENSORHELPER_H
#define TENSORHELPER_H

#include "tensorflow/core/framework/tensor.h"

/// Some array conversion tools.
/// \author David Stutz
template<typename T, int NDIMS>
class TensorConversion {
public:

  /// Access the underlying data pointer of the tensor.
  /// \param tensor
  /// \return
  static T* AccessDataPointer(const tensorflow::Tensor &tensor) {
    // get underlying Eigen tensor
    auto tensor_map = tensor.tensor<T, NDIMS>();
    // get the underlying array
    auto array = tensor_map.data();
    return const_cast<T*>(array);
  }
};


class TensorWrapper {
public:
    virtual tensorflow::Tensor &tensor() const  = 0;
    virtual int ndim() const = 0;
    virtual tensorflow::DataType dataType() const = 0;
    virtual ~TensorWrapper() {}
};

template<typename T>
class TensorWrap2d : public TensorWrapper
{
public:
    TensorWrap2d(size_t batch_size, size_t n) {
        mBatchSize = batch_size; mN=n;
        tensorflow::DataType dt = tensorflow::DT_FLOAT;
        if (typeid(T)==typeid(float)) dt=tensorflow::DT_FLOAT;
        if (typeid(T)==typeid(int)) dt=tensorflow::DT_INT64;
        if (typeid(T)==typeid(unsigned short)) dt=tensorflow::DT_UINT16;
        if (typeid(T)==typeid(short int)) dt=tensorflow::DT_INT16;
        mDataType = dt;
        mT = new tensorflow::Tensor(dt, tensorflow::TensorShape({ static_cast<int>(mBatchSize), static_cast<int>(mN)}));
        mData = TensorConversion<T,2>::AccessDataPointer(*mT);
        mPrivateTensor=true;
    }
    TensorWrap2d(tensorflow::Tensor &tensor) {
        mBatchSize = tensor.dim_size(0);
        mN = tensor.dim_size(1);
        mT = &tensor;
        mData = TensorConversion<T,2>::AccessDataPointer(tensor);
        mPrivateTensor=false;
    }
    tensorflow::Tensor &tensor() const { return *mT; }
    size_t n() const  { return mN; }
    int ndim() const { return 2; }
    tensorflow::DataType dataType() const  { return mDataType; }
    T *example(size_t element) { return mData + element*mN; }

    ~TensorWrap2d() { if (mPrivateTensor) delete mT; }
private:
    tensorflow::DataType mDataType;

    bool mPrivateTensor;
    tensorflow::Tensor *mT;
    T *mData;
    size_t mBatchSize;
    size_t mN;
};

template<typename T>
class TensorWrap3d : public TensorWrapper
{
public:
    TensorWrap3d(size_t batch_size, size_t nx, size_t ny) {
        mBatchSize = batch_size; mNx=nx; mNy=ny;
        tensorflow::DataType dt = tensorflow::DT_FLOAT;
        if (typeid(T)==typeid(float)) dt=tensorflow::DT_FLOAT;
        if (typeid(T)==typeid(int)) dt=tensorflow::DT_INT64;
        if (typeid(T)==typeid(unsigned short)) dt=tensorflow::DT_UINT16;
        if (typeid(T)==typeid(short int)) dt=tensorflow::DT_INT16;

        mDataType = dt;

        mT = new tensorflow::Tensor(dt, tensorflow::TensorShape({ static_cast<int>(mBatchSize), static_cast<int>(mNx), static_cast<int>(mNy)}));
        mData = TensorConversion<T,3>::AccessDataPointer(*mT);
        mPrivateTensor = true;
    }
    TensorWrap3d(tensorflow::Tensor &tensor) {
        mBatchSize = tensor.dim_size(0);
        mNx = tensor.dim_size(1);
        mNy = tensor.dim_size(2);
        mT = &tensor;
        mData = TensorConversion<T,3>::AccessDataPointer(tensor);
        mPrivateTensor=false;
    }

     ~TensorWrap3d() { if (mPrivateTensor) delete mT; }
    tensorflow::Tensor &tensor() const { return *mT; }
    size_t nx() const { return mNx; }
    size_t ny() const {return mNy; }
    T *example(size_t element) { return mData + element*mNx*mNy; }
    T *row(size_t element, size_t x) { return mData + element*mNx*mNy+x*mNx; }

    int ndim() const { return 3; }
    tensorflow::DataType dataType() const  { return mDataType; }

private:
    tensorflow::DataType mDataType;
    bool mPrivateTensor;

    tensorflow::Tensor *mT;
    T *mData;
    size_t mBatchSize;
    size_t mNx;
    size_t mNy;
};



#endif // TENSORHELPER_H
