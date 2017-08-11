#ifndef PREDICTOR_H
#define PREDICTOR_H
#include <QString>

namespace tensorflow {
class Session;
class Tensor;
class Status;
}

class Predictor
{

public:
    Predictor();
    ~Predictor();
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

#endif // PREDICTOR_H
