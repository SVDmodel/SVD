#include "modelinterface.h"

#include <QThread>
#include <QMetaType>

#include "inferencedata.h"
#include "randomgen.h"
#include "model.h"
#include "batch.h"



ModelInterface::ModelInterface()
{
}

void ModelInterface::setup(QString fileName)
{
    // setup is called *after* the set up of the main model
    lg = spdlog::get("dnn");
    if (lg)
        lg->info("DNN Setup, config file: {}", fileName.toStdString());

    mBatchManager = std::unique_ptr<BatchManager>(new BatchManager());
    mBatchManager->setup();

}

void ModelInterface::doWork(Batch *batch, int packageId)
{

    // do the processing with DNN....
    batch->changeState(Batch::DNN);
    // now just fake:
    dummyDNN(batch);

    batch->changeState(Batch::Finished);

    lg->debug("finished data package {} (size={})", packageId, batch->usedSlots());

    emit workDone(batch, packageId);
}

void ModelInterface::dummyDNN(Batch *batch)
{
    for (int i=0;i<batch->usedSlots();++i) {
        InferenceData &id=batch->inferenceData(i);
        // just random ....
        const State &s = Model::instance()->states()->randomState();
        restime_t rt = Model::instance()->year()+irandom(1,12);
        id.setResult(s.id(), rt);

    }
    QThread::msleep(5);

}


