#ifndef PREDICTORTEST_H
#define PREDICTORTEST_H
#include <QString>

namespace tensorflow {
class Session;
class Tensor;
class Status;
}

class PredictorTest
{

public:
    PredictorTest();
    ~PredictorTest();
    bool setup(QString model_path);
    // inception
    QString classifyImage(QString image_path);
    //
    QString insight();
    QString runModel();
private:
    tensorflow::Status getTopClasses(const tensorflow::Tensor &classes, const int n_top, tensorflow::Tensor *indices, tensorflow::Tensor *scores);
    tensorflow::Session *session;
    std::string label_file;


};

#endif // PREDICTORTEST_H
