#include "dnn.h"

#include "settings.h"
#include "model.h"
#include "tools.h"
#include "tensorhelper.h"
#include "batch.h"
#include "batchmanager.h"
#include "randomgen.h"

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

#pragma warning(pop)


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
    top_k_session = nullptr;
    mTopK_tf = false; // TODO: more dynamic
}

DNN::~DNN()
{
    if (session)
        session->Close();
    if (top_k_session)
        top_k_session->Close();
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
    //opts.config.set_inter_op_parallelism_threads(8); // no big effect.... but uses more threads
    //opts.config.set_intra_op_parallelism_threads(8);
    session = tensorflow::NewSession(opts); // no specific options: tensorflow::SessionOptions()

    lg->trace("attempting to load the graph...");
    Status load_graph_status = LoadGraph(file, session);
    if (!load_graph_status.ok()) {
        lg->error("Error loading the graph: {}", load_graph_status.error_message().data());
        return false;
    }
    lg->trace("ok!");

    if (mTopK_tf) {
        lg->trace("build the top-k graph...");
        // output_classes = new tensorflow::Tensor(dt, tensorflow::TensorShape({ static_cast<int>(mBatchSize), static_cast<int>(1418)}));
        //output_classes = new tensorflow::Input();
        int bs = BatchManager::instance()->batchSize();
        // number of classes:
        int ncls = static_cast<int>(Model::instance()->states()->states().size());
        Tensor top_k_tensor(tensorflow::DT_FLOAT, tensorflow::TensorShape({bs, ncls}));
        //top_k_tensor = new tensorflow::Tensor(tensorflow::DT_FLOAT, tensorflow::PartialTensorShape({-1, ncls}));
        //new tensorflow::Tensor(dt, tensorflow::TensorShape({ static_cast<int>(mBatchSize), static_cast<int>(mRows), static_cast<int>(mCols)}));

        const int n_top = 10; // TODO variable

        auto root = tensorflow::Scope::NewRootScope();
        string output_name = "top_k";
        tensorflow::GraphDefBuilder b;
        //string topk_input_name = "topk_input";

        // tensorflow::Node* topk =
        tensorflow::ops::TopK tk(root.WithOpName(output_name), top_k_tensor, n_top);


        lg->trace("top-k-tensor: {}", top_k_tensor.DebugString());
        //lg->trace("node: {}", topk->DebugString());

        // This runs the GraphDef network definition that we've just constructed, and
        // returns the results in the output tensors.
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
    }


    lg->info("DNN Setup complete.");
    return true;
#endif

}

class STimer {
public:
    STimer(std::shared_ptr<spdlog::logger> logger, std::string name) { start_time = std::chrono::system_clock::now(); _logger=logger; _name=name; }
    int elapsed() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - start_time).count(); }
    void print(std::string s) { _logger->debug("Timer {}: {}: {}us", _name, s, elapsed()); }
    ~STimer() { _logger->debug("Timer {}: {} us.", _name, elapsed()); }
private:
    std::shared_ptr<spdlog::logger> _logger;
    std::string _name;
    std::chrono::system_clock::time_point start_time;
};

Batch * DNN::run(Batch *batch)
{
    const int top_n=10;
    std::vector<Tensor> outputs;
    STimer timr(lg, "DNN::run");
    lg->debug("DNN: started execution for package {}.", batch->packageId());

    std::vector<std::pair<string, Tensor> > inputs;
    const std::list<InputTensorItem> &tdef = BatchManager::instance()->tensorDefinition();
    int tindex=0;
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
        for (int i=0;i<batch->usedSlots();++i) {
            InferenceData &id=batch->inferenceData(i);
            // just random ....
            const State &s = Model::instance()->states()->randomState();
            restime_t rt = Model::instance()->year()+irandom(1,12);
            id.setResult(s.id(), rt);

        }
        batch->changeState(Batch::FinishedDNN);
        return batch;
    }

    /* Run Tensorflow */
    timr.print("before main dnn");
    Status run_status = session->Run(inputs, {"out/Softmax", "time_out/Softmax"}, {}, &outputs);
    if (!run_status.ok()) {
        lg->trace("{}", batch->inferenceData(0).dumpTensorData());
        lg->error("Tensorflow error (run main network): {}", run_status.error_message());
        batch->setError(true);
        return batch;
    }
    timr.print("main dnn");

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
        scores = new Tensor(tensorflow::DT_FLOAT, tensorflow::TensorShape({  batch->batchSize(), top_n}));
        indices = new Tensor(tensorflow::DT_INT32, tensorflow::TensorShape({  batch->batchSize(), top_n}));

        // run the top-k on CPU
        getTopClasses(outputs[0], batch->batchSize(), top_n, indices, scores);
        timr.print("topk cpu");


    }



    lg->debug("DNN result: {} output tensors. package {}", outputs.size(), batch->packageId());
    lg->debug("out:  {}", outputs[0].DebugString());
    lg->debug("time: {}", outputs[1].DebugString());
    // output tensors: 2dim; 1x batch, 1x data
    // lg->debug("dimension time: {} x {}.", outputs[1].dim_size(0), outputs[1].dim_size(0));


    TensorWrap2d<float> out_time(outputs[1]);
    TensorWrap2d<float> scores_flat(*scores);
    TensorWrap2d<int32> indices_flat(*indices);


    // Now select for each example the result of the prediction
    // choose randomly from the result
    for (int i=0; i<batch->usedSlots(); ++i) {
        InferenceData &id = batch->inferenceData(i);
        // residence time: at least one year
        restime_t rt = static_cast<restime_t>( chooseProbabilisticIndex(out_time.example(i), static_cast<int>(out_time.n())) ) + 1;
        if (rt == out_time.n()) {
            // the state will be the same for the next period (no change)
            id.setResult(id.state(), rt);
        } else {
            // select the next state probalistically
            // the next state is not allowed to stay the same
            int self_index = -1;
            int this_state_index = id.state() - 1; // 0-based
            for (int j=0;j<indices_flat.n();++j) {
                if (indices_flat.example(i)[j] == this_state_index) {
                    self_index = j;
                    break;
                }
            }
            int index = chooseProbabilisticIndex(scores_flat.example(i), static_cast<int>(scores_flat.n()), self_index ) ;
            int state_index = indices_flat.example(i)[index];
            state_t stateId = Model::instance()->states()->stateByIndex( state_index ).id();
            id.setResult(stateId, rt);
        }
    }

    if (lg->should_log(spdlog::level::trace)) {

        lg->trace("{}", batch->inferenceData(0).dumpTensorData());
        InferenceData &id = batch->inferenceData(0);

        std::stringstream s;
        s << "Residence time\n";
        float *t = out_time.example(0);
        for (int i=0; i<outputs[1].dim_size(1);++i)
            s << i+1 << " yrs: " << (*t++)*100 << "%" << (id.nextTime() - Model::instance()->year()==i+1 ? " ***": "")<< "\n";

        s << "Next update in year: " << id.nextTime();
        s << "\nClassifcation Results\n";
        for (int i=0;i <scores_flat.n(); ++i) {
            s << indices_flat.example(0)[i] << ": " <<  scores_flat.example(0)[i]*100.
              << "% "<< Model::instance()->states()->stateByIndex(indices_flat.example(0)[i]).asString() << " id: "
              << Model::instance()->states()->stateByIndex(indices_flat.example(0)[i]).id() << "\n";
        }
        s << "Selected State: " << id.nextState() << " : " << Model::instance()->states()->stateById(id.nextState()).asString();
        lg->trace("Results for example 0 in the batch:");
        lg->trace("{}", s.str());


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

void DNN::getTopClasses( tensorflow::Tensor &classes, const int batch_size, const int n_top, tensorflow::Tensor *indices, tensorflow::Tensor *scores)
{
    std::priority_queue< std::pair<float, size_t>, std::vector<std::pair<float, size_t> >, ComparisonClassTopK > queue;

    lg->debug("CPU-TopK: Classes: x={}, y={}, Indices: x={}, y={}", classes.dim_size(0), classes.dim_size(1), indices->dim_size(0), indices->dim_size(1));
    lg->debug("classes : {}", classes.DebugString());
    lg->debug("indicies: {}", indices->DebugString());
    lg->debug("scores  : {}", scores->DebugString());

    int n_cls = classes.dim_size(1);
    TensorWrap2d<float> cls_dat(classes);
    TensorWrap2d<int32> res_ind(*indices);
    TensorWrap2d<float> res_scores(*scores);
    for (int i=0; i<batch_size; i++) {

        float *p = cls_dat.example(i);
        for (int j=0; j<n_cls; ++j, ++p) {
            if (queue.size()<n_top || *p > queue.top().first) {
                if (queue.size() == n_top)
                    queue.pop();
                queue.push( std::pair<float, size_t>(*p,j));
            }
        }
        // write back results... and empty the queue
        int j=0;
        while( !queue.empty() ) {
            res_ind.example(i)[j] = queue.top().second; // the index
            res_scores.example(i)[j] = queue.top().first; // the score
            queue.pop();
            ++j;
        }

    }
    if (lg->should_log(spdlog::level::trace)) {
        // output details for the first example.....
        lg->trace("Top-K-calculation (CPU):");
        std::stringstream s;
        for (int i=0;i<n_cls;++i)
            s << cls_dat.example(0)[i] << ", ";

        lg->trace("{}", s.str());
        for (int i=0;i<n_top;++i) {
            lg->trace("Index: {}, Score: {} %", res_ind.example(0)[i], res_scores.example(0)[i]*100.);
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
        p_sum+= (i!=skip_index ? values[i] : 0. );

    double p = nrandom(0., p_sum);

    p_sum = 0.;
    for (int i=0;i<n;++i, ++values) {
        p_sum += (i!=skip_index ? *values : 0. );
        if (p < p_sum)
            return i;
    }
    return n-1;
}



