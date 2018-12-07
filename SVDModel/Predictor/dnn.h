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
#ifndef DNN_H
#define DNN_H

#include "spdlog/spdlog.h"
namespace tensorflow { // forward declarations...
class Session;
class Tensor;
class Status;
class Input;
}
class Batch; // forward

#include "inputtensoritem.h"
#include "tensorhelper.h"

class DNN
{
public:
    DNN();
    ~DNN();
    /// access to the currently avaialable global instance of the DNN
    /// this allows accessing the model with DNN::instance()->....
    static DNN *instance() {
        assert(mInstance!=nullptr);
        return mInstance; }


    /// set up the actual DNN (TensorFlow)
    bool setupDNN();

    /// set up the links to the main model
    void setupInput();

    void setupBatch(Batch *abatch, std::vector<TensorWrapper*> &tensors);

    /// DNN main function: execute the DNN inference for the
    /// examples provided in 'batch'.
    Batch *run(Batch *abatch);

    // getters
    /// the definition of the tensors to fill
    const std::list<InputTensorItem> &tensorDefinition() const {return mTensorDef; }


private:
    static DNN *mInstance;


    TensorWrapper *buildTensor(size_t batch_size, InputTensorItem &item);
    // logging
    std::shared_ptr<spdlog::logger> lg;

    // DNN specifics
    bool mDummyDNN; ///< if true, then the tensorflow components are not really used (for debug builds)
    bool mTopK_tf; ///< use tensorflow for the state top k calculation
    size_t mTopK_NClasses; ///< number of classes used for the top k algorithm
    std::vector<std::string> mOutputTensorNames; ///< names of the output tensors (e.g. output/Softmax)
    size_t mNStateCls; ///< number of output classes for state
    size_t mNResTimeCls; ///< number of classes for residence time
    tensorflow::Status getTopClassesOldCode(const tensorflow::Tensor &classes, const int n_top, tensorflow::Tensor *indices, tensorflow::Tensor *scores);

    /// retrieve the top n classes in "classes" and store results in 'indices' and 'scores'.
    /// this function uses CPU (and not tensorflow)
    void getTopClasses(tensorflow::Tensor &classes, const size_t batch_size, const size_t n_top, tensorflow::Tensor *indices, tensorflow::Tensor *scores);
    tensorflow::Session *session;
    tensorflow::Session *top_k_session;


    /// select randomly an index 0..n-1, with values the weights.
    int chooseProbabilisticIndex(float *values, int n, int skip_index=-1);


    /// definition of input tensors
    std::list<InputTensorItem> mTensorDef;


};

#endif // DNN_H
