#include "dnninterface.h"

#include <QThread>
#include <QMetaType>
#include <QCoreApplication>

#include "inferencedata.h"
#include "randomgen.h"
#include "model.h"
#include "batch.h"



DNNInterface::DNNInterface()
{

}

void DNNInterface::setup(QString fileName)
{
    // setup is called *after* the set up of the main model
    lg = spdlog::get("dnn");
    if (lg)
        lg->info("DNN Setup, config file: {}", fileName.toStdString());

    mBatchManager = std::unique_ptr<BatchManager>(new BatchManager());
    mBatchManager->setup();

    mDNN = std::unique_ptr<DNN>(new DNN());
    if (mDNN->setup()) {
        mState=ModelRunState::ReadyToRun;
        emit dnnState("startup.ok");
    } else {
        mState=ModelRunState::ErrorDuringSetup;
        emit dnnState("startup.error");
    }


}

void DNNInterface::doWork(Batch *batch, int packageId)
{

    if (Model::instance()->state().shouldCancel()) {
        batch->setError(true);
        mState=ModelRunState::Stopping;
        emit workDone(batch, packageId);
        return;
    }

    // do the processing with DNN....
    batch->changeState(Batch::DNN);
    mState = ModelRunState::Running;

    if (!mDNN->run(batch)) {
        mState = ModelRunState::Error;
        emit dnnState("error");
        QCoreApplication::processEvents();
        return;
    }

    // now just fake:

    dummyDNN(batch);

    batch->changeState(Batch::Finished);

    lg->debug("finished data package {} (size={})", packageId, batch->usedSlots());

    emit workDone(batch, packageId);
    mState = ModelRunState::ReadyToRun;
    QCoreApplication::processEvents();
}

void DNNInterface::dummyDNN(Batch *batch)
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


