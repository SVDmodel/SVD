#include "predtest.h"

#include <iomanip>

#include "tensorhelper.h"
#include "spdlog/spdlog.h"

#pragma warning(push, 0)
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#pragma warning(pop)

using tensorflow::string;
using tensorflow::int32;


PredTest::PredTest()
{

}

void PredTest::testTensor()
{
    auto console = spdlog::get("main");

    TensorWrap2d<float> *tw = new TensorWrap2d<float>(6, 4);
    for (int i=0;i<6;++i) {
        for (int j=0;j<4;++j)
            tw->example(i)[j] = i*100 + j;
    }
    std::stringstream ss;
    for (int i=0;i<6;++i) {
        ss << "Example " << i << ": ";
        for (int j=0;j<4;++j)
            ss << std::setw(8) << tw->example(i)[j];
        ss << std::endl;
    }
    console->debug("{}", ss.str());
    delete tw;

    TensorWrapper *tw2 = new TensorWrap2d<float>(6, 4);
    std::vector<TensorWrapper*> twlist;
    twlist.push_back(tw2);

    TensorWrap2d<float> *tw3 = static_cast<TensorWrap2d<float>*>(twlist[0]);

    for (int i=0;i<6;++i) {
        for (int j=0;j<4;++j)
            tw3->example(i)[j] = i*100 + j;
    }

    ss.str(""); // clear
    for (int i=0;i<6;++i) {
        ss << "Example " << i << ": ";
        for (int j=0;j<4;++j)
            ss << std::setw(8) << tw3->example(i)[j];
        ss << std::endl;
    }
    console->debug("{}", ss.str());

    //
    TensorWrapper *w3 = new TensorWrap3d<float>(4,2,2);
    //TensorWrap3d<float> *tw3d = new TensorWrap3d<float>(4,2,2);
    TensorWrap3d<float> *tw3d = static_cast<TensorWrap3d<float>*>(w3);
    // put data in
    float *src = tw3->example(1);
    float *dest = tw3d->example(1);
    for (int i=0;i<4;++i)
        *dest++ = *src++;
    memcpy(tw3d->example(2), tw3->example(2),4*sizeof(float));

    ss.str(""); // clear
    for (int i=0;i<6;++i) {
        ss << "Example " << i << ": ";
        for (int j=0;j<2;++j) {
            for (int k=0;k<2;++k)
                ss << std::setw(8) << tw3d->row(i,j)[k];
            ss << " * ";
        }

        ss << std::endl;
    }
    console->debug("{}", ss.str());

    delete tw3d;
}

void PredTest::testDevicePlacement()
{
    auto console = spdlog::get("main");
    const int tensor_size = 100;
    tensorflow::Status tf_status;


    // TODO(jeff,opensource): This should really be a more interesting
    // computation.  Maybe turn this into an mnist model instead?
    tensorflow::Scope root = tensorflow::Scope::NewRootScope();
    using namespace ::tensorflow::ops;  // NOLINT(build/namespaces)
    auto x = Placeholder(root.WithOpName("x"), tensorflow::DT_FLOAT,
                         Placeholder::Shape({ -1,1 }));
    //auto v = Const(root, { { 3.f },{ -1.f } } );
    auto tmp = Add(root, x, x);
    auto sm = Softmax(root.WithOpName("y"), tmp);
    // A = [3 2; -1 0].  Using Const<float> means the result will be a
    // float tensor even though the initializer has integers.
    //  auto a = Const<float>(root, {{3, 2}, {-1, 0}});

    //  // x = [1.0; 1.0]
    //  auto x = Const(root.WithOpName("x"), {{1.f}, {1.f}});

    //  // y = A * x
    //  auto y = MatMul(root.WithOpName("y"), a, x);

    //  // y2 = y.^2
    //  auto y2 = Square(root, y);

    //  // y2_sum = sum(y2).  Note that you can pass constants directly as
    //  // inputs.  Sum() will automatically create a Const node to hold the
    //  // 0 value.
    //  auto y2_sum = Sum(root, y2, 0);

    //  // y_norm = sqrt(y2_sum)
    //  auto y_norm = Sqrt(root, y2_sum);

    //  // y_normalized = y ./ y_norm
    //  Div(root.WithOpName("y_normalized"), y, y_norm);

    tensorflow::GraphDef def;
    tf_status = root.ToGraphDef(&def);
    if (!tf_status.ok()) {
        console->error("Error create graph: {}", tf_status.error_message());
        return;
    }


    tensorflow::SessionOptions options;
    options.config.set_log_device_placement(true);
    std::unique_ptr<tensorflow::Session> session(tensorflow::NewSession(options));
    // GraphDef def = CreateGraphDef(opts);
    //tensorflow::graph::SetDefaultDevice("/device:GPU:0", &def);


    tf_status = session->Create(def);
    if (!tf_status.ok()) {
        console->error("Error create session: {}", tf_status.error_message());
        return;
    }
    const int M = 5;

    std::unique_ptr<tensorflow::thread::ThreadPool> step_threads(
                new tensorflow::thread::ThreadPool(tensorflow::Env::Default(), "trainer", M));

    for (int step = 0; step < M; ++step) {
        step_threads->Schedule([&session, step, tensor_size, &console]() {
            // Randomly initialize the input.
            tensorflow::Tensor x(tensorflow::DT_FLOAT, tensorflow::TensorShape({tensor_size, 1}));
            auto x_flat = x.flat<float>();
            x_flat.setRandom();
            //Eigen::Tensor<float, 0, Eigen::RowMajor> inv_norm =
            //    x_flat.square().sum().sqrt().inverse();
            //x_flat = x_flat * inv_norm();

            // Iterations.
            tensorflow::Status tf_status;
            std::vector<tensorflow::Tensor> outputs;
            for (int iter = 0; iter < 10; ++iter) {
                outputs.clear();
                tf_status = session->Run({{"x", x}}, {"y:0"}, {}, &outputs);
                if (!tf_status.ok()) {
                    console->error("Error run: {}", tf_status.error_message());
                    return;
                }
                //CHECK_EQ(size_t{2}, outputs.size());

                const tensorflow::Tensor& y = outputs[0];
                //const Tensor& y_norm = outputs[1];
                // Print out lambda, x, and y.
                //std::printf("%06d/%06d %s\n", session_index, step,
                //            DebugString(x, y).c_str());
                console->info("....");
                //std::printf("%06d/%06d %s\n", session_index, step,
                //            "grrrr");
                // Copies y_normalized to x.
                // x = y_norm;
            }
        });
    }

    // Delete the threadpool, thus waiting for all threads to complete.
    step_threads.reset(nullptr);
    TF_CHECK_OK(session->Close());



}
