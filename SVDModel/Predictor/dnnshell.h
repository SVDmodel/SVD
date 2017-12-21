#ifndef MODELINTERFACE_H
#define MODELINTERFACE_H

#include <QObject>
#include <QFutureWatcher>
#include <QThreadPool>
#include "spdlog/spdlog.h"

#include "modelrunstate.h"
#include "batchmanager.h"
#include "dnn.h"

class InferenceData; // forward
class Batch; // forward

class DNNShell: public QObject
{
    Q_OBJECT
public:
    DNNShell();
    ~DNNShell();

    bool isRunnig();
    int batchesProcessed() const { return mBatchesProcessed; }
    int cellsProcessed() const { return mCellsProcessed; }

private:
public slots:
    void setup(QString fileName);
    void doWork(Batch *batch);

    void dnnFinished(void *vbatch);
signals:
    void workDone(Batch *batch);

private:
    /// thread pool for running the DNN
    QThreadPool *mThreads;

    // loggers
    std::shared_ptr<spdlog::logger> lg;

    std::unique_ptr<BatchManager> mBatchManager;
    std::unique_ptr<DNN> mDNN;
    std::atomic<int> mProcessing;

    // store a watcher and a flag if the watcher is used (=true) or free (false)
    //std::vector<std::pair<QFutureWatcher<Batch*>*, Batch*> > mWatchers;
    int mBatchesProcessed;
    int mCellsProcessed;

};

#endif // MODELINTERFACE_H
