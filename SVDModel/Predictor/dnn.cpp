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
#include "dnn.h"

#include "settings.h"
#include "model.h"
#include "tools.h"
#include "tensorhelper.h"
#include "batch.h"
#include "batchdnn.h"
#include "batchmanager.h"
#include "randomgen.h"
#include "settings.h"
#include "fetchdata.h"

// Tensorflow includes
//  from inception demo
#include <fstream>
#include <vector>
#include <iomanip>

#include <queue>

#pragma warning(push, 0)
//Some includes with unfixable warnings: https://stackoverflow.com/questions/2541984/how-to-suppress-warnings-in-external-headers-in-visual-c

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
#include "tensorflow/core/framework/op_kernel.h"

#pragma warning(pop)

// CUDA Profiling
// #define CUDA_PROFILING
#ifdef CUDA_PROFILING
#include "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v8.0/include/cuda_profiler_api.h"
#endif

std::list<InputTensorItem> DNN::mTensorDef; // static def

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



DNN::DNN()
{

    if (spdlog::get("dnn"))
        spdlog::get("dnn")->debug("DNN created: {}", static_cast<void*>(this));
    session = nullptr;
    top_k_session = nullptr;
    mTopK_tf = true;
    mTopK_NClasses = 10;
    mNResTimeCls = 0; mNStateCls = 0;
}

DNN::~DNN()
{
    if (session)
        session->Close();
    if (top_k_session)
        top_k_session->Close();
}

bool DNN::setupDNN(size_t aindex)
{
#ifdef CUDA_PROFILING
    cudaProfilerStop();
#endif

    lg = spdlog::get("dnn");
    mIndex = aindex;
    auto settings = Model::instance()->settings();
    if (!lg)
        throw std::logic_error("DNN::setup: logging not available.");
    lg->info("Setup of DNN #{}", aindex);
    settings.requiredKeys("dnn", {"file", "maxBatchQueue", "topKNClasses", "state.name", "state.N", "restime.name", "restime.N"});

    std::string file = Tools::path(settings.valueString("dnn.file"));
    mTopK_tf = settings.valueBool("dnn.topKGPU", "true");
    mTopK_NClasses = settings.valueUInt("dnn.topKNClasses", 10);
    mOutputTensorNames = { settings.valueString("dnn.state.name"), settings.valueString("dnn.restime.name")};
    mNStateCls = settings.valueUInt("dnn.state.N");
    if (mNStateCls==0)
         mNStateCls = Model::instance()->states()->states().size(); // default: number of states

    mNResTimeCls = settings.valueUInt("dnn.restime.N");


    lg->info("DNN file: '{}'", file);

    if (lg->should_log(spdlog::level::debug)) {
        lg->debug("Definition of DNN-Output layers: State-Layer: '{}', '{}' classes.", mOutputTensorNames[0], mNStateCls);
        lg->debug("Definition of DNN-Output layers: Residence-Time-Layer: '{}', '{}' classes.", mOutputTensorNames[1], mNResTimeCls);
        lg->debug("Use of Top-K: running on GPU: '{}',  with '{}' classes.", mTopK_tf, mTopK_NClasses);
    }


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
    // log of device placement if log level debug is on
    if (lg->should_log(spdlog::level::debug))
        opts.config.set_log_device_placement(true);

    //opts.config.set_inter_op_parallelism_threads(16); // no big effect.... but uses more threads
    //opts.config.set_intra_op_parallelism_threads(16);
    opts.config.mutable_gpu_options()->set_allow_growth(true); // do not allocate all the RAM

    session = tensorflow::NewSession(opts); // no specific options: tensorflow::SessionOptions()

    lg->trace("attempting to load the graph...");
    Status load_graph_status = LoadGraph(file, session);
    if (!load_graph_status.ok()) {
        lg->error("Error loading the graph: {}", load_graph_status.error_message().data());
        return false;
    }
    lg->trace("Successfully loaded graph!");

    if (mTopK_tf) {
        lg->trace("build the top-k graph...");
        // output_classes = new tensorflow::Tensor(dt, tensorflow::TensorShape({ static_cast<int>(mBatchSize), static_cast<int>(1418)}));
        //output_classes = new tensorflow::Input();
        int bs = static_cast<int>( BatchManager::instance()->batchSize() );
        // number of classes:
        int ncls = static_cast<int>( mNStateCls );
        Tensor top_k_tensor(tensorflow::DT_FLOAT, tensorflow::TensorShape({bs, ncls}));
        //top_k_tensor = new tensorflow::Tensor(tensorflow::DT_FLOAT, tensorflow::PartialTensorShape({-1, ncls}));
        //new tensorflow::Tensor(dt, tensorflow::TensorShape({ static_cast<int>(mBatchSize), static_cast<int>(mRows), static_cast<int>(mCols)}));


        auto root = tensorflow::Scope::NewRootScope();
        string output_name = "top_k";
        tensorflow::GraphDefBuilder b;
        //string topk_input_name = "topk_input";

        // tensorflow::Node* topk =
        tensorflow::ops::TopK tk(root.WithOpName(output_name), top_k_tensor, static_cast<int>(mTopK_NClasses));


        lg->trace("top-k-tensor: {}", top_k_tensor.DebugString());
        //lg->trace("node: {}", topk->DebugString());

        tensorflow::GraphDef graph;
        Status tf_status;
        tf_status = root.ToGraphDef(&graph);

        if (!tf_status.ok()) {
            lg->error("Error building top-k graph definition: {}", tf_status.error_message());
            return false;
        }

        lg->trace("TK NODES") ;
        for (tensorflow::Node* node : root.graph()->nodes()) {
            lg->trace("Node {}: {}",node->id(), node->DebugString());
        }

        top_k_session = tensorflow::NewSession(tensorflow::SessionOptions());

        tf_status = top_k_session->Create(graph);
        //tf_status = session->Create(graph);
        if (!tf_status.ok()) {
            lg->error("Error creating top-k graph: {}", tf_status.error_message());
            return false;
        }


        // TEST
//        auto tsess = tensorflow::NewSession(opts);
//        auto troot = tensorflow::Scope::NewRootScope();
//        auto tscope = troot.WithDevice("/device:GPU:0");
//        tensorflow::GraphDef tgraph;

//        tensorflow::ops::Softmax sm(tscope.WithOpName("tsoftmax"),top_k_tensor);
//        tf_status = tscope.ToGraphDef(&tgraph);
//        if (!tf_status.ok())
//            lg->error("Error building graph definition: {}", tf_status.error_message());
//        tf_status = tsess->Create(tgraph);
//        if (!tf_status.ok())
//            lg->error("Error creating graph in session: {}", tf_status.error_message());

//        lg->trace("TK NODES2") ;
//        for (tensorflow::Node* node : troot.graph()->nodes()) {
//            lg->trace("Node {}: {}",node->id(), node->DebugString());
//        }
//        std::vector< Tensor > tmp;
//        tf_status = tsess->Run({ {"Const/Const" , top_k_tensor} }, {"tsoftmax:0"},
//        {}, &tmp);
//        if (!tf_status.ok())
//            lg->error("Error running: {}", tf_status.error_message());

//        tensorflow::LogAllRegisteredKernels();
    }

    // setup output: store ptr only when output is enabled
    double sleep_time=0.;
    while (Model::instance()->outputManager()==nullptr || Model::instance()->outputManager()->isSetup()==false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); sleep_time+=0.05;
        if (sleep_time>60) { // one minute
            lg->error("DNN setup: output manager not ready after 60 sec - time out.");
            return false;
        }
    }

    lg->info("DNN Setup complete.");
    return true;
#endif

}

class STimer {
public:
    STimer(std::shared_ptr<spdlog::logger> logger, std::string name) { start_time = std::chrono::system_clock::now(); _logger=logger; _name=name; }
    size_t elapsed() { return static_cast<size_t>( std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - start_time).count() ); }
    void print(std::string s) { _logger->debug("[{}] Timer {}: {}: {}us", std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()).time_since_epoch().count(),  _name, s, elapsed()) ; }
    void now() { _logger->debug("Timepoint: {}us", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() ));}
    ~STimer() { print("destroyed"); }
private:
    std::shared_ptr<spdlog::logger> _logger;
    std::string _name;
    std::chrono::system_clock::time_point start_time;
};



Batch * DNN::run(Batch *abatch)
{
    BatchDNN *batch = dynamic_cast<BatchDNN*>(abatch);
    if (!batch)
        throw std::logic_error("DNN:run: invalid Batch!");
#ifdef CUDA_PROFILING
    cudaProfilerStart();
#endif
    std::vector<Tensor> outputs;
    STimer timr(lg, "DNN::run:" + to_string(batch->packageId()));
    lg->debug("DNN#{}: started execution for package {}.", mIndex, batch->packageId());

    std::vector<std::pair<string, Tensor> > inputs;
    const std::list<InputTensorItem> &tdef = tensorDefinition();
    size_t tindex=0;
    for (const auto &def : tdef) {
        inputs.push_back( std::pair<string, Tensor>( def.name, batch->tensor(tindex)->tensor() ));
        tindex++;
    }

    // if disabled (in debug mode), TF_DEBUG_MODE
    if (mDummyDNN) {
        lg->debug("DNN in debug mode... no action");
        // wait a bit...
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // ... and produce a random result
        for (size_t i=0;i<batch->usedSlots();++i) {
            InferenceData &id=batch->inferenceData(i);
            // just random ....
            const State &s = Model::instance()->states()->randomState();
            restime_t rt = static_cast<restime_t>(Model::instance()->year()+irandom(1,12));
            id.setResult(s.id(), rt);

        }
        batch->changeState(Batch::FinishedDNN);
        return batch;
    }

    /* Run Tensorflow */
    timr.print("before main dnn");
    //timr.now();

    Status run_status = session->Run(inputs, mOutputTensorNames, {}, &outputs);
    if (!run_status.ok()) {
        lg->trace("{}", batch->inferenceData(0).dumpTensorData());
        lg->error("Tensorflow error (run main network): {}", run_status.error_message());
        batch->setError(true);
        return batch;
    }
    timr.print("main dnn");
    //timr.now();

    // test dimensions of the network
    if (outputs.size() != 2 || static_cast<size_t>(outputs[0].dim_size(1)) != mNStateCls || static_cast<size_t>(outputs[1].dim_size(1)) != mNResTimeCls ) {
        lg->error("Wrong number of dimensions of DNN outputs. Number of output tensors: '{}' (expected: 2), Classes state: '{}' (expected: {}); classes residence time: '{}' (expected: {}).",
                  outputs.size(), outputs.size()>0 ? outputs[0].dim_size(1) : 0, mNStateCls,
                  outputs.size()>1 ? outputs[1].dim_size(1) : 0 , mNResTimeCls);
        batch->setError(true);
        return batch;
    }

    tensorflow::Tensor *scores= nullptr;
    tensorflow::Tensor *indices = nullptr;
    std::vector< Tensor > topk_output;
    if (mTopK_tf) {
        // run top-k labels
        // top_k_session
        run_status = top_k_session->Run({ {"Const/Const" , outputs[0]} }, {"top_k:0", "top_k:1"},
        {}, &topk_output);
        if (!run_status.ok()) {
            lg->trace("{}", batch->inferenceData(0).dumpTensorData());
            lg->error("Tensorflow error (run top-k): {}", run_status.error_message());
            batch->setError(true);
            return batch;
        }
        timr.print("topk dnn");
        scores = &topk_output[0];
        indices = &topk_output[1];
    } else {
        // use CPU to extract top-k results
        // outputs[0] is the output tensor with the state probabilities
        scores = new Tensor(tensorflow::DT_FLOAT, tensorflow::TensorShape({  static_cast<long long>(batch->batchSize()), static_cast<long long>(mTopK_NClasses)}));
        indices = new Tensor(tensorflow::DT_INT32, tensorflow::TensorShape({  static_cast<long long>(batch->batchSize()), static_cast<long long>(mTopK_NClasses)}));

        // run the top-k on CPU
        getTopClasses(outputs[0], batch->batchSize(), mTopK_NClasses, indices, scores);
        timr.print("topk cpu");


    }

#ifdef CUDA_PROFILING
    cudaProfilerStop();
#endif


    lg->debug("DNN result (#{}): {} output tensors. package {}, {} slots.", mIndex, outputs.size(), batch->packageId(), batch->usedSlots());
    lg->debug("out:  {}", outputs[0].DebugString());
    lg->debug("time: {}", outputs[1].DebugString());
    // output tensors: 2dim; 1x batch, 1x data
    // lg->debug("dimension time: {} x {}.", outputs[1].dim_size(0), outputs[1].dim_size(0));


    TensorWrap2d<float> out_time(outputs[1]);
    TensorWrap2d<float> scores_flat(*scores);
    TensorWrap2d<int32> indices_flat(*indices);

    // Copy the results of the TopK (states, probabilities, residence times) to the batch
    for (size_t i=0; i<batch->usedSlots(); ++i) {
        float *ostate = scores_flat.example(i);
        float *tstate = batch->stateProbResult(i);
        int *oidx = indices_flat.example(i);
        state_t *tidx = batch->stateResult(i);
        for (size_t r=0;r<mTopK_NClasses;++r) {

            *tstate++ = *ostate++;
            // the result of TopK is the *index* within the input of the operation
            // the StateId starts with 1, i.e. to convert from the index (0-based).
            *tidx++ = Model::instance()->states()->stateByIndex(static_cast<size_t>(*oidx++)).id();
        }

        float *otime = out_time.example(i);
        float *ttime = batch->timeProbResult(i);
        for (size_t r=0;r<mNResTimeCls;++r) {
            *ttime++ = *otime++;
        }
    }


    // cleanup
    if (!mTopK_tf) {
        delete scores;
        delete indices;
    }

    lg->debug("DNN::run finished; package {}", batch->packageId());
    batch->changeState(Batch::FinishedDNN);
    return batch;
}

void DNN::setupInput()
{
    //    mTensorDef =  {
    //        {"test", InputTensorItem::DT_FLOAT, 2, 24, 10, InputTensorItem::Climate},
    //        {"test2", InputTensorItem::DT_INT16, 1, 1, 0, InputTensorItem::State}
    //    };
        // load tensor definitions from file
    mTensorDef.clear();
    std::string metafilename = Tools::path(Model::instance()->settings().valueString("dnn.metadata"));
    if (!Tools::fileExists(metafilename))
        throw std::logic_error("The metadata file for the DNN (" + metafilename + ") does not exist (specified as 'dnn.metadata')!");

    Settings mg;
    mg.loadFromFile(metafilename);

    auto sections = mg.findKeys("input.", true);
    std::shared_ptr<spdlog::logger> lg = spdlog::get("dnn");
    lg->debug("Found sections: {}", join(sections));
    if (lg->should_log(spdlog::level::trace)) {
        for (auto &s : sections) {
            auto keys = mg.findKeys("input." + s);
            lg->trace("Section: {}", s);
            for (auto &k : keys)
                lg->trace("'{}' = '{}'", k, mg.valueString(k));
        }
    }

    for (auto &s : sections) {
        mg.requiredKeys("input."+s, {"enabled", "dim", "sizeX", "sizeY", "dtype", "type"});
        if (!mg.valueBool("input." + s + ".enabled"))
            continue;

        InputTensorItem item(s,
                             mg.valueString("input."+ s +".dtype"),
                             mg.valueUInt("input." + s + ".dim"),
                             mg.valueUInt("input." + s + ".sizeX"),
                             mg.valueUInt("input." + s + ".sizeY"),
                             mg.valueString("input."+ s +".type"));

        mTensorDef.push_back(item);
        // setup the data extractor
        InputTensorItem *ti =& mTensorDef.back();
        ti->mFetch = FetchData::createFetchObject(ti);
        if (!ti->mFetch) {
            lg->error("create Batch for DNN: Error:");
            lg->error("Name: '{}', dataype: '{}', dimensions: {}, size-x: {}, size-y: {}, content: '{}'",
                      item.name, item.datatypeString(item.type), item.ndim, item.sizeX, item.sizeY, item.contentString(item.content));
            throw std::logic_error("Could not create a tensor (check the logfile).");
        }
        ti->mFetch->setup(&mg, "input." + s, item);
    }


    //    mTensorDef =  {
    //        // {"clim_input", "float", 2, 10, 40, "Climate"}, // GPP Climate
    //        {"clim_input", "float", 2, 10, 24, "Climate"}, // monthly climate
    //        {"state_input", "int16", 1, 1, 0, "State"},
    //        {"time_input", "float", 1, 1, 0, "ResidenceTime"},
    //        {"site_input", "float", 1, 2, 0, "Site"} ,
    //        {"distance_input", "float", 1, 1, 0, "DistanceOutside"}, // distance to the forested area outside
    //        {"neighbor_input", "float", 1, 62, 0, "Neighbors"},
    //        {"keras_learning_phase", "bool", 0, 0, 0, "Scalar"}
    //    };


    if (lg->should_log(spdlog::level::debug)) {
        lg->debug("Available data types: {}", InputTensorItem::allDataTypeStrings());
        lg->debug("Available content types: {}", InputTensorItem::allContentStrings());
        // print tensor-items
        lg->debug("InputTensorItems:");
        for (auto &i : mTensorDef)
            lg->debug("Name: '{}', dataype: '{}', dimensions: {}, size-x: {}, size-y: {}, content: '{}'",
                      i.name, i.datatypeString(i.type), i.ndim, i.sizeX, i.sizeY, i.contentString(i.content));
    }

}

TensorWrapper *DNN::buildTensor(size_t batch_size, InputTensorItem &item)
{
    TensorWrapper *tw = nullptr;

    // a scalar, i.e. one value for the whole *batch*
    if (item.ndim == 0) {
        switch (item.type) {
        case InputTensorItem::DT_BOOL: {
            tw = new TensorWrap1d<bool>();
            // defaults to true, TODO
            TensorWrap1d<bool> *twb = static_cast< TensorWrap1d<bool>* >(tw);
            twb->setValue(false);
            spdlog::get("main")->debug("created a scalar, value: '{}'", twb->value());
            break;
        }
        default: break;
        }

    }

    // a 1d vector per example (or a single value per example)
    if (item.ndim == 1) {
        switch (item.type) {
        case InputTensorItem::DT_FLOAT:
            tw = new TensorWrap2d<float>(batch_size, item.sizeX); break;
        case InputTensorItem::DT_INT16:
            tw = new TensorWrap2d<short int>(batch_size, item.sizeX); break;
        case InputTensorItem::DT_UINT16:
            tw = new TensorWrap2d<short unsigned int>(batch_size, item.sizeX); break;
        case InputTensorItem::DT_INT64:
            tw = new TensorWrap2d<long long>(batch_size, item.sizeX); break;
        case InputTensorItem::DT_INT32:
            tw = new TensorWrap2d<int32_t>(batch_size, item.sizeX); break;

        default:
            throw std::logic_error("Unhandled data type in tensorwrapper");
        }
    }

    // a 2d vector by example
    if (item.ndim==2) {
        switch (item.type) {
        case InputTensorItem::DT_FLOAT:
            tw = new TensorWrap3d<float>(batch_size, item.sizeX, item.sizeY); break;
        default: throw std::logic_error("datatype not handled in tensorwrapper");
        }
    }

    if (tw)
        return tw;
    std::shared_ptr<spdlog::logger> lg = spdlog::get("main");
    lg->error("build Tensor: not able to create the tensor from the definition:");
    lg->error("Name: '{}', dataype: '{}', dimensions: {}, size-x: {}, size-y: {}, content: '{}'",
              item.name, item.datatypeString(item.type), item.ndim, item.sizeX, item.sizeY, item.contentString(item.content));
    throw std::logic_error("Could not create a tensor.");

}



void DNN::setupBatch(Batch *abatch, std::vector<TensorWrapper *> &tensors)
{
    BatchDNN *batch = dynamic_cast<BatchDNN*>(abatch);
    if (!batch)
        throw std::logic_error("DNN:run: invalid Batch!");

    // loop over tensor definition and create the required tensors....
    size_t index=0;
    for (auto &td : mTensorDef) {
        // create a tensor of the right size

        TensorWrapper *tw = buildTensor(batch->batchSize(), td);

        td.index = index++; // static_cast<int>(b->mTensors.size());
        tensors.push_back(tw);
    }


}

tensorflow::Status DNN::getTopClassesOldCode(const tensorflow::Tensor &classes, const int n_top, tensorflow::Tensor *indices, tensorflow::Tensor *scores)
{
    auto root = tensorflow::Scope::NewRootScope();
    using namespace ::tensorflow::ops;  // NOLINT(build/namespaces)

    string output_name = "top_k";
    TopK(root.WithOpName(output_name), classes, n_top);
    // This runs the GraphDef network definition that we've just constructed, and
    // returns the results in the output tensors.
    tensorflow::GraphDef graph;
    TF_RETURN_IF_ERROR(root.ToGraphDef(&graph));

    std::unique_ptr<tensorflow::Session> session(
        tensorflow::NewSession(tensorflow::SessionOptions()));
    TF_RETURN_IF_ERROR(session->Create(graph));
    // The TopK node returns two outputs, the scores and their original indices,
    // so we have to append :0 and :1 to specify them both.
    std::vector<Tensor> out_tensors;
    TF_RETURN_IF_ERROR(session->Run({}, {output_name + ":0", output_name + ":1"},
                                    {}, &out_tensors));
    *scores = out_tensors[0];
    *indices = out_tensors[1];

    return Status::OK();



}

class ComparisonClassTopK {
public:
    bool operator() (const std::pair<float, size_t> &p1, const std::pair<float, size_t> &p2) {
        //comparison code here: we want to keep track of the smallest element in the list
        return p1.first>p2.first;
    }
};

void DNN::getTopClasses( tensorflow::Tensor &classes, const size_t batch_size, const size_t n_top, tensorflow::Tensor *indices, tensorflow::Tensor *scores)
{
    std::priority_queue< std::pair<float, size_t>, std::vector<std::pair<float, size_t> >, ComparisonClassTopK > queue;

    lg->debug("CPU-TopK: Classes: x={}, y={}, Indices: x={}, y={}", classes.dim_size(0), classes.dim_size(1), indices->dim_size(0), indices->dim_size(1));
    lg->debug("classes : {}", classes.DebugString());
    lg->debug("indicies: {}", indices->DebugString());
    lg->debug("scores  : {}", scores->DebugString());

    size_t n_cls = static_cast<size_t>(classes.dim_size(1));
    TensorWrap2d<float> cls_dat(classes);
    TensorWrap2d<int32> res_ind(*indices);
    TensorWrap2d<float> res_scores(*scores);
    for (size_t i=0; i<batch_size; i++) {

        float *p = cls_dat.example(i);
        for (size_t j=0; j<n_cls; ++j, ++p) {
            if (queue.size()<n_top || *p > queue.top().first) {
                if (queue.size() == n_top)
                    queue.pop();
                queue.push( std::pair<float, size_t>(*p,j));
            }
        }
        // write back results... and empty the queue
        int j=0;
        while( !queue.empty() ) {
            res_ind.example(i)[j] = static_cast<tensorflow::int32>(queue.top().second); // the index
            res_scores.example(i)[j] = queue.top().first; // the score
            queue.pop();
            ++j;
        }

    }
    if (lg->should_log(spdlog::level::trace)) {
        // output details for the first example.....
        lg->trace("Top-K-calculation (CPU):");
        std::stringstream s;
        for (size_t i=0;i<n_cls;++i)
            s << cls_dat.example(0)[i] << ", ";

        lg->trace("{}", s.str());
        for (size_t i=0;i<n_top;++i) {
            lg->trace("Index: {}, Score: {} %", res_ind.example(0)[i], res_scores.example(0)[i]*100.f);
        }
    }

}

// choose randomly a value in *values (length=n), return the index.
// if 'skip_index' != -1, then this index is not allowed (and the skipped)
int DNN::chooseProbabilisticIndex(float *values, int n, int skip_index)
{

    // calculate the sum of probs
    double p_sum = 0.;
    for (int i=0;i<n;++i)
        p_sum+= (i!=skip_index ? static_cast<double>(values[static_cast<size_t>(i)]) : 0. );

    double p = nrandom(0., p_sum);

    p_sum = 0.;
    for (int i=0;i<n;++i, ++values) {
        p_sum += (i!=skip_index ? static_cast<double>(*values) : 0. );
        if (p < p_sum)
            return i;
    }
    return n-1;
}



