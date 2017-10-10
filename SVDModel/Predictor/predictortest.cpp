#include "predictortest.h"

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


#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

#include "spdlog/spdlog.h"

// These are all common classes it's handy to reference with no namespace.
using tensorflow::Flag;
using tensorflow::Tensor;
using tensorflow::Status;
using tensorflow::string;
using tensorflow::int32;

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

// Takes a file name, and loads a list of labels from it, one per line, and
// returns a vector of the strings. It pads with empty strings so the length
// of the result is a multiple of 16, because our model expects that.
Status ReadLabelsFile(string file_name, std::vector<string>* result,
                      size_t* found_label_count) {
  std::ifstream file(file_name);
  if (!file) {
    return tensorflow::errors::NotFound("Labels file ", file_name,
                                        " not found.");
  }
  result->clear();
  string line;
  while (std::getline(file, line)) {
    result->push_back(line);
  }
  *found_label_count = result->size();
  const int padding = 16;
  while (result->size() % padding) {
    result->emplace_back();
  }
  return Status::OK();
}

// Given an image file name, read in the data, try to decode it as an image,
// resize it to the requested size, and then scale the values as desired.
Status ReadTensorFromImageFile(string file_name, const int input_height,
                               const int input_width, const float input_mean,
                               const float input_std,
                               std::vector<Tensor>* out_tensors) {
  auto root = tensorflow::Scope::NewRootScope();
  using namespace ::tensorflow::ops;  // NOLINT(build/namespaces)

  string input_name = "file_reader";
  string output_name = "normalized";
  auto file_reader =
      tensorflow::ops::ReadFile(root.WithOpName(input_name), file_name);
  // Now try to figure out what kind of file it is and decode it.
  const int wanted_channels = 3;
  tensorflow::Output image_reader;
  if (tensorflow::StringPiece(file_name).ends_with(".png")) {
    image_reader = DecodePng(root.WithOpName("png_reader"), file_reader,
                             DecodePng::Channels(wanted_channels));
  } else if (tensorflow::StringPiece(file_name).ends_with(".gif")) {
    image_reader = DecodeGif(root.WithOpName("gif_reader"), file_reader);
  } else {
    // Assume if it's neither a PNG nor a GIF then it must be a JPEG.
    image_reader = DecodeJpeg(root.WithOpName("jpeg_reader"), file_reader,
                              DecodeJpeg::Channels(wanted_channels));
  }
  // Now cast the image data to float so we can do normal math on it.
  auto float_caster =
      Cast(root.WithOpName("float_caster"), image_reader, tensorflow::DT_FLOAT);
  // The convention for image ops in TensorFlow is that all images are expected
  // to be in batches, so that they're four-dimensional arrays with indices of
  // [batch, height, width, channel]. Because we only have a single image, we
  // have to add a batch dimension of 1 to the start with ExpandDims().
  auto dims_expander = ExpandDims(root, float_caster, 0);
  // Bilinearly resize the image to fit the required dimensions.
  auto resized = ResizeBilinear(
      root, dims_expander,
      Const(root.WithOpName("size"), {input_height, input_width}));
  // Subtract the mean and divide by the scale.
  Div(root.WithOpName(output_name), Sub(root, resized, {input_mean}),
      {input_std});

  // This runs the GraphDef network definition that we've just constructed, and
  // returns the results in the output tensor.
  tensorflow::GraphDef graph;
  TF_RETURN_IF_ERROR(root.ToGraphDef(&graph));

  std::unique_ptr<tensorflow::Session> session(
      tensorflow::NewSession(tensorflow::SessionOptions()));
  TF_RETURN_IF_ERROR(session->Create(graph));
  TF_RETURN_IF_ERROR(session->Run({}, {output_name}, {}, out_tensors));
  return Status::OK();
}

// Reads a model graph definition from disk, and creates a session object you
// can use to run it.
Status PLoadGraph(string graph_file_name,
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


// Analyzes the output of the Inception graph to retrieve the highest scores and
// their positions in the tensor, which correspond to categories.
Status GetTopLabels(const std::vector<Tensor>& outputs, int how_many_labels,
                    Tensor* indices, Tensor* scores) {
  auto root = tensorflow::Scope::NewRootScope();
  using namespace ::tensorflow::ops;  // NOLINT(build/namespaces)

  string output_name = "top_k";
  TopK(root.WithOpName(output_name), outputs[0], how_many_labels);
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

// Given the output of a model run, and the name of a file containing the labels
// this prints out the top five highest-scoring values.
Status PrintTopLabels(const std::vector<Tensor>& outputs,
                      string labels_file_name, std::vector<string> &res) {
  std::vector<string> labels;
  size_t label_count;
  Status read_labels_status =
      ReadLabelsFile(labels_file_name, &labels, &label_count);
  if (!read_labels_status.ok()) {
    LOG(ERROR) << read_labels_status;
    return read_labels_status;
  }
  const int how_many_labels = std::min(5, static_cast<int>(label_count));
  Tensor indices;
  Tensor scores;
  TF_RETURN_IF_ERROR(GetTopLabels(outputs, how_many_labels, &indices, &scores));
  tensorflow::TTypes<float>::Flat scores_flat = scores.flat<float>();
  tensorflow::TTypes<int32>::Flat indices_flat = indices.flat<int32>();
  for (int pos = 0; pos < how_many_labels; ++pos) {
    const int label_index = indices_flat(pos);
    const float score = scores_flat(pos);
    string line=labels[label_index] + " (" + std::to_string(label_index) + "): " + std::to_string(score);
    res.push_back(line);
    //LOG(INFO) << labels[label_index] << " (" << label_index << "): " << score;
  }
  return Status::OK();
}

// This is a testing function that returns whether the top label index is the
// one that's expected.
Status CheckTopLabel(const std::vector<Tensor>& outputs, int expected,
                     bool* is_expected) {
  *is_expected = false;
  Tensor indices;
  Tensor scores;
  const int how_many_labels = 1;
  TF_RETURN_IF_ERROR(GetTopLabels(outputs, how_many_labels, &indices, &scores));
  tensorflow::TTypes<int32>::Flat indices_flat = indices.flat<int32>();
  if (indices_flat(0) != expected) {
    LOG(ERROR) << "Expected label #" << expected << " but got #"
               << indices_flat(0);
    *is_expected = false;
  } else {
    *is_expected = true;
  }
  return Status::OK();
}

/*
Predictor::Predictor()
{
    std::vector<Tensor> resized_tensors;
    Status read_tensor_status =
        ReadTensorFromImageFile("test.jpg", 299, 299, 0,
                                255, &resized_tensors);
    if (!read_tensor_status.ok()) {
      LOG(ERROR) << read_tensor_status.error_message().data();
      //return QString::fromStdString(read_tensor_status.error_message());
    }
} */


PredictorTest::PredictorTest()
{

    session=0;
}

PredictorTest::~PredictorTest()
{
    if (session) {
        session->Close();
        delete session;
    }
}

bool PredictorTest::setup(QString model_path)
{

    if (session) {
        session->Close();
        delete session;
    }
    // We need to call this to set up global state for TensorFlow.
    //char *argv[]={"inception"};
    //int argc=1;
    //tensorflow::port::InitMain(argv[0], &argc, &argv);
    //if (argc > 1) {
    //  LOG(ERROR) << "Unknown argument " << argv[1] << "\n" << usage;
    //  return -1;
    //}

    tensorflow::SessionOptions opts;
    opts.config.set_log_device_placement(true);
    session = tensorflow::NewSession(tensorflow::SessionOptions()); // no specific options: tensorflow::SessionOptions()

    string graph_path = model_path.toStdString();

    label_file = QFileInfo(model_path).dir().filePath("imagenet_slim_labels.txt").toStdString();
    qDebug() << "label-file:" << label_file.data();


    //std::unique_ptr<tensorflow::Session> session;
    //string graph_path = tensorflow::io::JoinPath(root_dir, graph);
    Status load_graph_status = PLoadGraph(graph_path, session);
    if (!load_graph_status.ok()) {
      qWarning() << load_graph_status.error_message().data();
      return false;
    }
    return true;
}

QString PredictorTest::classifyImage(QString image_path)
{
    // wraps the inception logic... and returns a text description...
    // Get the image from disk as a float array of numbers, resized and normalized
    // to the specifications the main graph expects.
    int32 input_width = 299;
    int32 input_height = 299;
    int32 input_mean = 0;
    int32 input_std = 255;
    string input_layer = "input";
    string output_layer = "InceptionV3/Predictions/Reshape_1";

    std::vector<Tensor> resized_tensors;
    string simage_path = image_path.toStdString();
    Status read_tensor_status =
        ReadTensorFromImageFile(simage_path, input_height, input_width, input_mean,
                                input_std, &resized_tensors);
    if (!read_tensor_status.ok()) {
      qWarning() << read_tensor_status.error_message().data();
      return QString::fromStdString(read_tensor_status.error_message());
    }
    const Tensor& resized_tensor = resized_tensors[0];

    // Actually run the image through the model.
    std::vector<Tensor> outputs;
    Status run_status = session->Run({{input_layer, resized_tensor}},
                                     {output_layer}, {}, &outputs);
    if (!run_status.ok()) {
      qWarning() << "Running model failed: " << run_status.error_message().data();
      return QString::fromStdString(run_status.error_message());
    }

    // now retrieve the top-5 labels
    std::vector<string> out_text;
    Status print_status = PrintTopLabels(outputs, label_file, out_text);
    if (!print_status.ok()) {
      qWarning() << "Running print failed: " << print_status.error_message().data();
      return QString::fromStdString(print_status.error_message());
    }
    QString res;
    for (const string& s : out_text) {
        res += QString::fromStdString(s) + "\n";
    }
    return res;

}

QString PredictorTest::insight()
{

    //# https://joe-antognini.github.io/machine-learning/windows-tf-project
    std::vector<float> data = std::vector<float>(4*40, 0.f);
    auto mapped_x = Eigen::TensorMap<Eigen::Tensor<float, 2> >(&data[0],4,40);
    auto eigen_x = Eigen::Tensor<float,2>(mapped_x);


    Tensor Input1(tensorflow::DT_FLOAT, tensorflow::TensorShape({4,40}));
    //Input1.tensor<float,2>() = eigen_x;


    //    auto mapped_X_ = Eigen::TensorMap<Eigen::Tensor<float, 2, Eigen::RowMajor>>
    //                         (&data[0], 2, 2);
    //      auto eigen_X_ = Eigen::Tensor<float, 2, Eigen::RowMajor>(mapped_X_);

    //      Tensor X_(DT_FLOAT, TensorShape({ 2, 2 }));
    //      X_.tensor<float, 2>() = eigen_X_;

    //Setup Input Tensors

    Tensor Input0(tensorflow::DT_FLOAT, tensorflow::TensorShape({1,1}));

    // Output
    //std::vector output;

    auto t = Input1.tensor<float,2>();
    const Eigen::Tensor<float, 2>::Dimensions& d = t.dimensions();
    QStringList out;
    out  << QString("Dim size: %1, dim0: %2 dim1: %3 ").arg(int(d.size)).arg(int(d[0])).arg(int(d[1])) ;
    out << QString("n dimensionsXXX: %1").arg(t.NumDimensions);

    size_t dx=d[0], dy=d[1];
    float sum=0.f;
    for (size_t x=0;x<dx;++x)
        for (size_t y=0;y<dy;++y)
            t(x,y) = 0.f;


    //t.setContstant(0.f);
    t(2,20) = 10.f;
    t(1, 10) = 5.f;

    for (size_t x=0;x<dx;++x)
        for (size_t y=0;y<dy;++y)
            sum += t(x,y);

    out << QString("sum: %1").arg(sum);

    //std::cout << "Dims " << t.NumDimensions;

    //Input1.scalar<float>()() = 1.0;
    //Input0.scalar<float>()() = 0.0;



    // trying to create and fill a Tensorflow tensor, with at least 4 dimensions
    // to see how the data is stored in memory;
    // hopefully able to convert it to the Eigen tensor and from there to an array.
    const int batch_size = 1;
    const int depth = 5;
    const int height = 5;
    const int width = 5;
    const int channels = 3;
    Tensor tensor(tensorflow::DT_INT32, tensorflow::TensorShape({batch_size, depth, height, width, channels}));

    // get underlying Eigen tensor
    auto tensor_map = tensor.tensor<int, 5>();

    // fill and print the tensor
    QString s;
    for (int n = 0; n < batch_size; n++) {
        for (int d = 0; d < depth; d++) {
            std::cout << d << " --" << std::endl;
            for (int h = 0; h < height; h++) {
                for (int w = 0; w < width; w++) {
                    for (int c = 0; c < channels; c++) {
                        tensor_map(n, d, h, w, c) = (((n*depth + d)*height + h)*width + w)*channels + c;
                        s+=QString("%1,").arg(tensor_map(n, d, h, w, c));

                    }
                  s += " ** ";
                }
                s+= "\n";
            }
        }
    }
    out << s;

    // get the underlying array
    auto array = tensor_map.data();
    int* int_array = static_cast<int*>(array);

    s="";
    // try to print the same to see the data layout
    for (int n = 0; n < batch_size; n++) {
        for (int d = 0; d < depth; d++) {
            out << QString::number(d);
            for (int h = 0; h < height; h++) {
                for (int w = 0; w < width; w++) {
                    for (int c = 0; c < channels; c++) {

                       s +=QString("%1, ").arg( int_array[(((n*depth + d)*height + h)*width + w)*channels + c] );
                    }
                   s+= " ";
                }
                out << s; s="";
            }
        }
    }

    Tensor tensor2(tensorflow::DT_FLOAT, tensorflow::TensorShape({4,5}));
    float *idata = TensorConversion<float,2>::AccessDataPointer(tensor2);
    for (int i=0;i<20;++i)
        idata[i] = i;
    out << " **** ";
    auto my_map = tensor2.tensor<float, 2>();
    out << QString("%1 - %2").arg(my_map(0,0)).arg(my_map(0,4));


    return out.join("\n");
}

template<typename T>
class PTensorWrap2d
{
public:
    PTensorWrap2d(size_t batch_size, size_t n) {
        mBatchSize = batch_size; mN=n;
        tensorflow::DataType dt = tensorflow::DT_FLOAT;
        if (typeid(T)==typeid(float)) dt=tensorflow::DT_FLOAT;
        if (typeid(T)==typeid(int)) dt=tensorflow::DT_INT64;
        if (typeid(T)==typeid(unsigned short)) dt=tensorflow::DT_UINT16;
        if (typeid(T)==typeid(short int)) dt=tensorflow::DT_INT16;

        mT = new Tensor(dt, tensorflow::TensorShape({ static_cast<int>(mBatchSize), static_cast<int>(mN)}));
        mData = TensorConversion<T,2>::AccessDataPointer(*mT);
        mPrivateTensor=true;
    }
    PTensorWrap2d(Tensor &tensor) {
        mBatchSize = tensor.dim_size(0);
        mN = tensor.dim_size(1);
        mT = &tensor;
        mData = TensorConversion<T,2>::AccessDataPointer(tensor);
        mPrivateTensor=false;
    }
    Tensor &tensor() const { return *mT; }
    size_t n() const  { return mN; }
    T *example(size_t element) { return mData + element*mN; }

    ~PTensorWrap2d() { if (mPrivateTensor) delete mT; }
private:
    bool mPrivateTensor;
    Tensor *mT;
    T *mData;
    size_t mBatchSize;
    size_t mN;
};

template<typename T>
class PTensorWrap3d
{
public:
    PTensorWrap3d(size_t batch_size, size_t nx, size_t ny) {
        mBatchSize = batch_size; mNx=nx; mNy=ny;
        tensorflow::DataType dt = tensorflow::DT_FLOAT;
        if (typeid(T)==typeid(float)) dt=tensorflow::DT_FLOAT;
        if (typeid(T)==typeid(int)) dt=tensorflow::DT_INT64;
        if (typeid(T)==typeid(unsigned short)) dt=tensorflow::DT_UINT16;

        mT = new Tensor(dt, tensorflow::TensorShape({ static_cast<int>(mBatchSize), static_cast<int>(mNx), static_cast<int>(mNy)}));
        mData = TensorConversion<T,3>::AccessDataPointer(*mT);
    }
    Tensor &tensor() const { return *mT; }
    size_t nx() const { return mNx; }
    size_t ny() const {return mNy; }
    T *example(size_t element) { return mData + element*mNx*mNy; }
    T *row(size_t element, size_t x) { return mData + element*mNx*mNy+x*mNx; }

    ~PTensorWrap3d() { delete mT; }
private:
    Tensor *mT;
    T *mData;
    size_t mBatchSize;
    size_t mNx;
    size_t mNy;
};


QString PredictorTest::runModel()
{
    const int batchsize=5;
    const int timesteps=10;
    const int Nneighbors=62;
    const int top_n = 10;

    PTensorWrap2d<short int> state(batchsize, 1);
    PTensorWrap2d<float> time(batchsize, 1);
    PTensorWrap3d<float> climate(batchsize, timesteps, 40);
    PTensorWrap2d<float> neighbors(batchsize, Nneighbors);
    PTensorWrap2d<float> site(batchsize, 2);

    for (int i=0;i<batchsize;++i) {
        *state.example(i) = i;
        float *d = climate.example(i);
        for (int j=0;j<climate.nx()*climate.ny();++j)
            d[j] = 0.f + j / 10.;
        site.example(i)[0]=32.3f;
        site.example(i)[1]=44.2f;
        for (int j=0;j<Nneighbors;++j)
            neighbors.example(i)[j] = 0.f;

        *time.example(i) = 1;
    }

    /*return( {'state_input': state,
            'site_input': sitedata,
            'time_input': restime,
            'neighbor_input': neighbors,
            'clim_input': cdat},
            { 'out': labs,
             'time_out': labs_time })
sinput, climinput, siteinput, neighborinput, timeinput

Blas GEMM launch failed --> close python session with active tensorflow!!
*/
    Tensor learning_phase(tensorflow::DT_BOOL, tensorflow::TensorShape());
    learning_phase.scalar<bool>()()=0.f;


    std::vector<Tensor> outputs;

    std::vector<std::pair<string, Tensor> > inputs = {{"state_input", state.tensor()},
                                                      {"clim_input", climate.tensor()},
                                                      {"site_input", site.tensor()},
                                                      {"neighbor_input", neighbors.tensor()},
                                                      {"time_input", time.tensor()},
                                                      {"dropout_1/keras_learning_phase", learning_phase}
                                                     };
    Status run_status = session->Run(inputs, {"out/Softmax", "time_out/Softmax"}, {}, &outputs);
    if (!run_status.ok()) {
        qWarning() << "Running model failed: " << run_status.error_message().data();
        return QString::fromStdString(run_status.error_message());
    }

    QStringList out;
    // now analyze the resulting tensors
    qDebug() << outputs.size() << "output tensors";
    qDebug() << QString::fromStdString(outputs[0].DebugString());
    qDebug() << QString::fromStdString(outputs[1].DebugString());
    // output tensors: 2dim; 1x batch, 1x data
    qDebug() << outputs[1].dim_size(0) << " x " << outputs[1].dim_size(1);

    // dump
    PTensorWrap2d<float> out_time(outputs[1]);
    for (int i=0;i<batchsize;++i) {
        QString line = QString("Example %1: ").arg(i);
        for (int j=0;j<out_time.n();j++)
            line+=QString("%1,").arg(out_time.example(i)[j]);
        out << line;
    }


    // top labels

    Tensor indices;
    Tensor scores;
    run_status = getTopClasses(outputs[0], top_n, &indices, &scores);
    if (!run_status.ok()) {
        qWarning() << "Running top-k failed: " << run_status.error_message().data();
        return QString::fromStdString(run_status.error_message());
    }

    out << "Classifcation Results";
    PTensorWrap2d<float> scores_flat(scores);
    PTensorWrap2d<int32> indicies_flat(indices);
    for (int i=0;i<batchsize;++i) {
        out << QString("**** Example %1 ***** ").arg(i);
        for (int j=0;j<out_time.n();j++)
            out << QString("State: %1 prob: %2").arg( indicies_flat.example(i)[j] ).arg( scores_flat.example(i)[j] );
    }






    for (int i=0;i<batchsize;++i) {
        out << QString("Example %1").arg(i);
        out << QString("state: %1 site 1: %2 site 2: %3").arg(*state.example(i)).arg(site.example(i)[0]).arg(site.example(i)[1]);
        out << QString("climate: v1: %1 v40/1: %2 v40/10 %3").arg(climate.example(i)[0]).arg(climate.example(i)[climate.nx()-1]).arg(climate.row(i,9)[climate.nx()-1]);
    }

    return out.join("\n");

}

void PredictorTest::tensorTest()
{

    auto console = spdlog::get("main");

    PTensorWrap2d<float> *tw = new PTensorWrap2d<float>(6, 4);
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

}

Status PredictorTest::getTopClasses(const Tensor &classes, const int n_top, Tensor *indices, Tensor *scores)
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
