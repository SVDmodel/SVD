#ifndef MODELINTERFACE_H
#define MODELINTERFACE_H

#include <QObject>
#include "spdlog/spdlog.h"

#include "batchmanager.h"

class InferenceData; // forward

class ModelInterface: public QObject
{
    Q_OBJECT
public:
    ModelInterface();



private:
public slots:
    void setup(QString fileName);

    void doWork(std::list<InferenceData*>*package, int packageId);
signals:
    void workDone(std::list<InferenceData*>*, int packageId);

private:
    void dummyDNN(std::list<InferenceData *> *package);
    // loggers
    std::shared_ptr<spdlog::logger> lg;

    std::unique_ptr<BatchManager> mBatchManager;

};

#endif // MODELINTERFACE_H
