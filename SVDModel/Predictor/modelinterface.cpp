#include "modelinterface.h"

#include <QThread>

#include "inferencedata.h"
#include "randomgen.h"
#include "model.h"

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

void ModelInterface::doWork(std::list<InferenceData *> *package, int packageId)
{

    // do the processing with DNN....
    // now just fake:
    dummyDNN(package);

    lg->debug("finished data package {} (size={})", packageId, package->size());

    emit workDone(package, packageId);
}

void ModelInterface::dummyDNN(std::list<InferenceData *> *package)
{
    for (InferenceData *d : *package) {
        // just random ....
        const State &s = Model::instance()->states()->randomState();
        restime_t rt = Model::instance()->year()+irandom(1,12);
        d->setResult(s.id(), rt);
    }
}


