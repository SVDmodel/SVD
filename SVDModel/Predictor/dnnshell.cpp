#include "dnnshell.h"

#include <QThread>
#include <QMetaType>
#include <QCoreApplication>
#include <QtConcurrent>
#include <QFutureWatcher>

#include "inferencedata.h"
#include "randomgen.h"
#include "model.h"
#include "batch.h"



DNNShell::DNNShell()
{
    mThreads = new QThreadPool();
}

DNNShell::~DNNShell()
{
    delete mThreads;

}


void DNNShell::setup(QString fileName)
{
    // setup is called *after* the set up of the main model
    lg = spdlog::get("dnn");
    if (lg)
        lg->info("DNN Setup, config file: {}", fileName.toStdString());

    RunState::instance()->dnnState()=ModelRunState::Creating;

    mBatchManager = std::unique_ptr<BatchManager>(new BatchManager());
    mBatchManager->setup();

    mDNN = std::unique_ptr<DNN>(new DNN());
    if (!mDNN->setup()) {
        RunState::instance()->dnnState()=ModelRunState::ErrorDuringSetup;
        return;
    }
    int n_threads = Model::instance()->settings().valueInt("dnn.threads", -1);
    if (n_threads>-1) {
        mThreads->setMaxThreadCount(n_threads);
        lg->debug("setting DNN threads to {}.", n_threads);
    }
    lg->debug("Thread pool for DNN: using {} threads.", mThreads->maxThreadCount());
    mBatchesProcessed = 0;
    mCellsProcessed = 0;
    mProcessing = 0;

    RunState::instance()->dnnState()=ModelRunState::ReadyToRun;


}

void DNNShell::doWork(Batch *batch, int packageId)
{

    batch->setPackageId(packageId);
    RunState::instance()->dnnState() = ModelRunState::Running;

    if (RunState::instance()->cancel()) {
        batch->setError(true);
        //RunState::instance()->dnnState()=ModelRunState::Stopping; // TODO: main
        emit workDone(batch, packageId);
        return;
    }

    // find a suitable watcher
    //QFutureWatcher<Batch*> *watcher=getFutureWatcher(batch);
    //QFuture<Batch*> dnn_result;
    batch->changeState(Batch::DNN);
    mProcessing++;
    lg->debug("DNNShell: received package {}. Starting DNN (batch: {}, state: {}, active threads now: {}, #processing: {}) ", batch->packageId(), (void*)batch, batch->state(), mThreads->activeThreadCount(), mProcessing);
    QtConcurrent::run( mThreads, [this, batch]{  mDNN->run(batch);

                                                if (!QMetaObject::invokeMethod(this, "dnnFinished", Qt::QueuedConnection,
                                                                          Q_ARG(void*, static_cast<void*>(batch))) )
                                                    batch->setError(true);
                                                            });
    //watcher->setFuture(dnn_result);





    //QCoreApplication::processEvents();
}

void DNNShell::dnnFinished(void *vbatch)
{
    Batch * batch = static_cast<Batch*>(vbatch);
    mProcessing--;
    // get the batch from the future watcher
    //Batch *batch = getFinishedBatch();
    //QFutureWatcher<Batch*> *watcher = static_cast< QFutureWatcher<Batch*> * >(sender());
    if (!batch) {
        RunState::instance()->setError("Error in DNN (no watcher)", RunState::instance()->dnnState());
        return;
    }


    if (batch->hasError()) {
        RunState::instance()->setError("Error in DNN", RunState::instance()->dnnState());
        emit workDone(batch, batch->packageId());
        QCoreApplication::processEvents();
        return;
    }

    lg->debug("DNNShell: dnn finished, package {}, {} cells. Sending to main.", batch->packageId(), batch->usedSlots());

    mBatchesProcessed++;
    mCellsProcessed += batch->usedSlots();

    batch->changeState(Batch::Finished);

    lg->debug("finished data package {} (size={})", batch->packageId(), batch->usedSlots());

    emit workDone(batch, batch->packageId());
    if (!isRunnig())
        RunState::instance()->dnnState() = ModelRunState::ReadyToRun;


}



bool DNNShell::isRunnig()
{
    return mProcessing > 0;
}


