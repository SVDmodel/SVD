#include "modelcontroller.h"
#include "modelshell.h"
#include "toymodel.h"

ModelController::ModelController(QObject *parent) : QObject(parent)
{
    ModelShell *model_shell = new ModelShell;
    modelThread = new QThread();
    mModel = model_shell;
    mModel->moveToThread(modelThread);

    connect(modelThread, &QThread::finished, model_shell, &QObject::deleteLater);

    //connect(this, &ModelController::operate, model_shell, &ModelShell::doWork);
    //connect(model_shell, &ModelShell::resultReady, this, &ModelController::handleResults);

    dnnThread = new QThread();
    ToyInference *ti = new ToyInference;
    ti->moveToThread(dnnThread);
    connect(dnnThread, &QThread::finished, ti, &QObject::deleteLater);

    // connection between main model and DNN:
    connect(&mModel->toyModel(), &ToyModel::newPackage, ti, &ToyInference::doWork);
    connect(ti, &ToyInference::workDone, &mModel->toyModel(), &ToyModel::processedPackage);
    connect(&mModel->toyModel(), &ToyModel::finished, this, &ModelController::finishedRun);

    // logging
    connect(mModel, &ModelShell::log, this, &ModelController::log);
    log("Modelcontroller: before thread.start()");
    modelThread->setObjectName("SVDMain");
    modelThread->start();

    dnnThread->setObjectName("SVDDNN");
    dnnThread->start();
}

ModelController::~ModelController()
{
    abort();
    modelThread->quit();
    dnnThread->quit();
    modelThread->wait();
    dnnThread->wait();
    log("Destroyed Thread");

}

void ModelController::run()
{
    QMetaObject::invokeMethod(mModel, "run", Qt::QueuedConnection);
    log("... ModelController: started");
}

void ModelController::abort()
{
    QMetaObject::invokeMethod(mModel, "abort");
    log(".... stopping thread....");

}

void ModelController::finishedRun()
{
    log(" *** Finished ***");
}
