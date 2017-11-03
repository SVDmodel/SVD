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
    for (auto &f : mWatchers)
        delete f.first;
    mWatchers.clear();
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
    lg->debug("Thread pool for DNN: using {} threads.", mThreads->maxThreadCount());
    mBatchesProcessed = 0;
    mCellsProcessed = 0;

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
    QFutureWatcher<Batch*> *watcher=getFutureWatcher();
    QFuture<Batch*> dnn_result;
    batch->changeState(Batch::DNN);
    lg->debug("DNNShell: received package {}. Starting DNN (batch: {}, state: {}) ", batch->packageId(), (void*)batch, batch->state());
    dnn_result = QtConcurrent::run( mThreads, [this, batch]{ return mDNN->run(batch);});
    watcher->setFuture(dnn_result);





    //QCoreApplication::processEvents();
}

void DNNShell::dnnFinished()
{
    // get the batch from the future watcher
    QFutureWatcher<Batch*> *watcher = getFinishedWatcher();
    //QFutureWatcher<Batch*> *watcher = static_cast< QFutureWatcher<Batch*> * >(sender());
    if (!watcher) {
        RunState::instance()->setError("Error in DNN (no watcher)", RunState::instance()->dnnState());
        return;
    }


    Batch *batch = watcher->future().result();
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

    //QCoreApplication::processEvents();

}


std::mutex _watcher_mutex;
QFutureWatcher<Batch*> *DNNShell::getFutureWatcher()
{
    std::lock_guard<std::mutex> guard(_watcher_mutex);

    // return a pointer to a future watcher which is not busy right now.
    QFutureWatcher<Batch*> *fw=nullptr;
    for (std::pair<QFutureWatcher<Batch*>*, bool> &f : mWatchers) {
        if (f.second == false) {
            fw=f.first;
            f.second = true; // lock!
            //connect(fw, &QFutureWatcher<Batch*>::finished, this, &DNNShell::dnnFinished);
            break;
        }
    }
    if (fw)
        return fw;
    // create a new watcher
    fw=new QFutureWatcher<Batch*>();
    mWatchers.push_back(std::pair<QFutureWatcher<Batch*>*, bool>( fw, true ) );
    connect(fw, &QFutureWatcher<Batch*>::finished, this, &DNNShell::dnnFinished);
    lg->debug("Created FutureWatcher (list contains now {} watchers)", mWatchers.size());
    return fw;

}

QFutureWatcher<Batch *> *DNNShell::getFinishedWatcher()
{
    std::lock_guard<std::mutex> guard(_watcher_mutex);
    for (std::pair<QFutureWatcher<Batch*>*, bool> &f : mWatchers) {
        if (f.second == true && f.first->isFinished() && f.first->result() && f.first->result()->state()==Batch::FinishedDNN) {
            f.second = false; // unlock the watcher
            return f.first;
        }
    }
    // debug output
    lg->debug("Error in getFinishedWatcher: no watcher found. ");
    for (std::pair<QFutureWatcher<Batch*>*, bool> &f : mWatchers) {
        lg->debug("Lock: {}, watcher: {}, Batch: {}, Batchstate: {}", ( f.second?"true":"false"), (void*)f.first, (void*)f.first->result(),
                  int(f.first->result()?f.first->result()->state() : -1));
    }
    return nullptr;

}



bool DNNShell::isRunnig()
{
    for (auto f : mWatchers) {
        if (f.second == true)
            return true;
    }
    return false;
}


