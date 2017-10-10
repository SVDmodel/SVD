#ifndef MODELINTERFACE_H
#define MODELINTERFACE_H

#include <QObject>
#include "spdlog/spdlog.h"

#include "batchmanager.h"
#include "dnn.h"

class InferenceData; // forward
class Batch; // forward

class ModelInterface: public QObject
{
    Q_OBJECT
public:
    ModelInterface();



private:
public slots:
    void setup(QString fileName);

    void doWork(Batch *batch, int packageId);
signals:
    void workDone(Batch *batch, int packageId);
    void dnnState(QString msg);

private:
    void dummyDNN(Batch *batch);
    // loggers
    std::shared_ptr<spdlog::logger> lg;

    std::unique_ptr<BatchManager> mBatchManager;
    std::unique_ptr<DNN> mDNN;

};

#endif // MODELINTERFACE_H
