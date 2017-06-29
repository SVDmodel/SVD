#ifndef PREDICTOR_H
#define PREDICTOR_H
#include <QString>

namespace tensorflow {
class Session;
}

class Predictor
{

public:
    Predictor();
    ~Predictor();
    bool setup(QString model_path);
    // inception
    QString classifyImage(QString image_path);
private:
    tensorflow::Session *session;
    std::string label_file;


};

#endif // PREDICTOR_H
