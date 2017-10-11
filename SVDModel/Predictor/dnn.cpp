#include "dnn.h"

#include "settings.h"
#include "model.h"
#include "tools.h"
#include "tensorhelper.h"
#include "batch.h"
#include "batchmanager.h"

// Tensorflow includes
//  from inception demo
#include <fstream>
#include <vector>
#include <iomanip>

#include "tensorflow/cc/ops/const_op.h"
#include "tensorflow/cc/ops/image_ops.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"

// These are all common classes it's handy to reference with no namespace.
using tensorflow::Flag;
using tensorflow::Tensor;
using tensorflow::Status;
using tensorflow::string;
using tensorflow::int32;

// Reads a model graph definition from disk, and creates a session object you
// can use to run it.
Status LoadGraph(string graph_file_name,
                 tensorflow::Session* session) {
  tensorflow::GraphDef graph_def;
  Status load_graph_status =
      ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
  if (!load_graph_status.ok()) {
    return tensorflow::errors::NotFound("Failed to load compute graph at '",
                                        graph_file_name, "'");
  }
  //session->reset(tensorflow::NewSession(tensorflow::SessionOptions()));
  Status session_create_status = session->Create(graph_def);
  if (!session_create_status.ok()) {
    return session_create_status;
  }
  return Status::OK();
}


DNN *DNN::mInstance = 0;

DNN::DNN()
{
    if (mInstance!=nullptr)
        throw std::logic_error("Creation of DNN: instance ptr is not 0.");
    mInstance = this;
    if (spdlog::get("dnn"))
        spdlog::get("dnn")->debug("DNN created: {#x}", (void*)this);
    session = nullptr;
}

DNN::~DNN()
{
    mInstance=nullptr;
}

bool DNN::setup()
{
    lg = spdlog::get("dnn");
    auto settings = Model::instance()->settings();
    if (!lg)
        throw std::logic_error("DNN::setup: logging not available.");
    lg->info("Setup of DNN.");
    settings.requiredKeys("dnn", {"file", "maxBatchQueue"});

    std::string file = Tools::path(settings.valueString("dnn.file"));
    lg->info("DNN file: '{}'", file);


    // set-up of the DNN
    if (session) {
        lg->info("Session is already open... closing.");
        session->Close();
        delete session;
    }
#ifdef TF_DEBUG_MODE
    lg->info("*** debug build: Tensorflow is disabled.");
    mDummyDNN = true;
    return true;
#else
    mDummyDNN = false;
    tensorflow::SessionOptions opts;
    opts.config.set_log_device_placement(true);
    session = tensorflow::NewSession(tensorflow::SessionOptions()); // no specific options: tensorflow::SessionOptions()

    lg->trace("attempting to load the graph...");
    Status load_graph_status = LoadGraph(file, session);
    if (!load_graph_status.ok()) {
        lg->error("Error loading the graph: {}", load_graph_status.error_message().data());
      return false;
    }

    lg->info("DNN Setup complete.");
    return true;
#endif

}

bool DNN::run(Batch *batch)
{
    std::vector<Tensor> outputs;

    std::vector<std::pair<string, Tensor> > inputs;
    const std::list<InputTensorItem> &tdef = BatchManager::instance()->tensorDefinition();
    int tindex=0;
    for (const auto &def : tdef) {
        inputs.push_back( std::pair<string, Tensor>( def.name, batch->tensor(tindex)->tensor() ));
        tindex++;
    }

    if (mDummyDNN) {
        lg->debug("DNN in debug mode... no action");
        // wait a bit...
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return true;
    }

    /* Run Tensorflow */
    Status run_status = session->Run(inputs, {"out/Softmax", "time_out/Softmax"}, {}, &outputs);
    if (!run_status.ok()) {
        lg->trace("{}", batch->inferenceData(0).dumpTensorData());
        lg->error("Tensorflow error: {}", run_status.error_message());
        batch->setError(true);
        return false;
    }

    lg->debug("DNN result: {} output tensors.", outputs.size());
    lg->debug("out:  {}", outputs[0].DebugString());
    lg->debug("time: {}", outputs[1].DebugString());
    // output tensors: 2dim; 1x batch, 1x data
    lg->debug("dimension time: {} x {}.", outputs[1].dim_size(0), outputs[1].dim_size(2));

    return true;
}



