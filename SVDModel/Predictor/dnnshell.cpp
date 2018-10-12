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
#include "batchmanager.h"
#include "dnn.h"



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
    mBatchesProcessed = 0;
    mCellsProcessed = 0;
    mProcessing = 0;

    // setup is called *after* the set up of the main model
    lg = spdlog::get("dnn");
    if (lg)
        lg->info("DNN Setup, config file: {}", fileName.toStdString());

    RunState::instance()->dnnState()=ModelRunState::Creating;

    try {
    mBatchManager = std::unique_ptr<BatchManager>(new BatchManager());
    mBatchManager->setup();
    } catch (const std::exception &e) {
        lg->error("An error occurred during setup of the Batch-Manager (DNN): {}", e.what());
        RunState::instance()->dnnState()=ModelRunState::ErrorDuringSetup;
        return;
    }

    try {
        mDNN = std::unique_ptr<DNN>(new DNN());
        if (!mDNN->setupDNN()) {
            RunState::instance()->dnnState()=ModelRunState::ErrorDuringSetup;
            return;
        }

        // wait for the model thread to complete model setup before
        // setting up the inputs (which may need data from the model)

        while (RunState::instance()->modelState() == ModelRunState::Creating) {
            lg->debug("waiting for Model thread thread...");
            QThread::msleep(50);
            QCoreApplication::processEvents();
        }

        mDNN->setupInput();

    } catch (const std::exception &e) {
        RunState::instance()->dnnState()=ModelRunState::ErrorDuringSetup;
        lg->error("An error occurred during DNN setup: {}", e.what());
        return;
    }
    int n_threads = Model::instance()->settings().valueInt("dnn.threads", -1);
    if (n_threads>-1) {
        mThreads->setMaxThreadCount(n_threads);
        lg->debug("setting DNN threads to {}.", n_threads);
    }
    lg->debug("Thread pool for DNN: using {} threads.", mThreads->maxThreadCount());

    RunState::instance()->dnnState()=ModelRunState::ReadyToRun;


}

void DNNShell::doWork(Batch *batch)
{

    RunState::instance()->dnnState() = ModelRunState::Running;

    if (RunState::instance()->cancel()) {
        batch->setError(true);
        //RunState::instance()->dnnState()=ModelRunState::Stopping; // TODO: main
        emit workDone(batch);
        return;
    }


    if (batch->state()!=Batch::Fill)
        lg->error("Batch {} [{}] is in the wrong state {}, size: {}", batch->packageId(), static_cast<void*>(batch), batch->state(), batch->usedSlots());

    batch->changeState(Batch::DNNInference);
    mProcessing++;
    lg->debug("DNNShell: received package {}. Starting DNN (batch: {}, state: {}, active threads now: {}, #processing: {}) ", batch->packageId(), static_cast<void*>(batch), batch->state(), mThreads->activeThreadCount(), mProcessing);

    QtConcurrent::run( mThreads,
                       [](DNNShell *shell, Batch *batch, DNN* dnn){
                            dnn->run(batch);

                            if (!QMetaObject::invokeMethod(shell, "dnnFinished", Qt::QueuedConnection,
                                                           Q_ARG(void*, static_cast<void*>(batch))) ) {
                                batch->setError(true);
                            }
                        }, this, batch, mDNN.get());
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
        emit workDone(batch);
        QCoreApplication::processEvents();
        return;
    }

    lg->debug("DNNShell: dnn finished, package {}, {} cells. Sending to main.", batch->packageId(), batch->usedSlots());

    mBatchesProcessed++;
    mCellsProcessed += batch->usedSlots();

    batch->changeState(Batch::Finished);

    lg->debug("finished data package {} [{}] (size={})", batch->packageId(), static_cast<void*>(batch), batch->usedSlots());

    emit workDone(batch);
    if (!isRunnig())
        RunState::instance()->dnnState() = ModelRunState::ReadyToRun;


}



bool DNNShell::isRunnig()
{
    return mProcessing > 0;
}


