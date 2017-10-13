#include "dnnshell.h"

#include <QThread>
#include <QMetaType>
#include <QCoreApplication>

#include "inferencedata.h"
#include "randomgen.h"
#include "model.h"
#include "batch.h"



DNNShell::DNNShell()
{

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
    if (mDNN->setup()) {
        RunState::instance()->dnnState()=ModelRunState::ReadyToRun;
    } else {
        RunState::instance()->dnnState()=ModelRunState::ErrorDuringSetup;
    }


}

void DNNShell::doWork(Batch *batch, int packageId)
{

    if (RunState::instance()->cancel()) {
        batch->setError(true);
        //RunState::instance()->dnnState()=ModelRunState::Stopping; // TODO: main
        emit workDone(batch, packageId);
        return;
    }

    // do the processing with DNN....
    batch->changeState(Batch::DNN);
    RunState::instance()->dnnState() = ModelRunState::Running;

    if (!mDNN->run(batch)) {
        RunState::instance()->setError("Error in DNN", RunState::instance()->dnnState());
        emit workDone(batch, packageId);
        QCoreApplication::processEvents();
        return;
    }

    // now just fake:
    // dummyDNN(batch);

    batch->changeState(Batch::Finished);

    lg->debug("finished data package {} (size={})", packageId, batch->usedSlots());

    emit workDone(batch, packageId);
    RunState::instance()->dnnState() = ModelRunState::ReadyToRun;
    //QCoreApplication::processEvents();
}

void DNNShell::dummyDNN(Batch *batch)
{
    // dump....
    if (lg->should_log(spdlog::level::trace))
        lg->trace("{}", batch->inferenceData(0).dumpTensorData());

    for (int i=0;i<batch->usedSlots();++i) {
        InferenceData &id=batch->inferenceData(i);
        // just random ....
        const State &s = Model::instance()->states()->randomState();
        restime_t rt = Model::instance()->year()+irandom(1,12);
        id.setResult(s.id(), rt);

    }
    QThread::msleep(5);

}


