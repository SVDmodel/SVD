#ifndef MODELINTERFACE_H
#define MODELINTERFACE_H

#include <QObject>
#include "spdlog/spdlog.h"

#include "batchmanager.h"

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

private:
    void dummyDNN(Batch *batch);
    // loggers
    std::shared_ptr<spdlog::logger> lg;

    std::unique_ptr<BatchManager> mBatchManager;

};

#endif // MODELINTERFACE_H
