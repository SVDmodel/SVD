#include "modelcontroller.h"
#include "modelshell.h"
#include "toymodel.h"

ToyModelController::ToyModelController(QObject *parent) : QObject(parent)
{
    ToyModelShell *model_shell = new ToyModelShell;
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
    connect(&mModel->toyModel(), &ToyModel::finished, this, &ToyModelController::finishedRun);

    // logging
    connect(mModel, &ToyModelShell::log, this, &ToyModelController::log);
    log("Modelcontroller: before thread.start()");
    modelThread->setObjectName("SVDMain");
    modelThread->start();

    dnnThread->setObjectName("SVDDNN");
    dnnThread->start();
}

ToyModelController::~ToyModelController()
{
    abort();
    modelThread->quit();
    dnnThread->quit();
    modelThread->wait();
    dnnThread->wait();
    log("Destroyed Thread");

}

void ToyModelController::run()
{
    QMetaObject::invokeMethod(mModel, "run", Qt::QueuedConnection);
    log("... ModelController: started");
}

void ToyModelController::abort()
{
    QMetaObject::invokeMethod(mModel, "abort");
    log(".... stopping thread....");

}

void ToyModelController::finishedRun()
{
    log(" *** Finished ***");
}


//***************** Model Controller ******************


ModelController::ModelController(QObject *parent)
{
    ModelShell *model_shell = new ModelShell;
    modelThread = new QThread();
    mModelShell = model_shell;
    mModelShell->moveToThread(modelThread);

    connect(modelThread, &QThread::finished, model_shell, &QObject::deleteLater);

    connect(mModelShell, &ModelShell::stateChanged, this, &ModelController::stateChanged);

    //connect(this, &ModelController::operate, model_shell, &ModelShell::doWork);
    //connect(model_shell, &ModelShell::resultReady, this, &ModelController::handleResults);

    dnnThread = new QThread();
    ToyInference *ti = new ToyInference;
    ti->moveToThread(dnnThread);
    connect(dnnThread, &QThread::finished, ti, &QObject::deleteLater);

    // connection between main model and DNN:
    //connect(&mModel->model(), &Model::newPackage, ti, &Inference::doWork);
    //connect(ti, &ToyInference::workDone, &mModel->model(), &Model::processedPackage);
    //connect(&mModel->model(), &Model::finished, this, &ModelController::finishedRun);

    // logging
    connect(mModelShell, &ModelShell::log, this, &ModelController::log);
    log("Modelcontroller: before thread.start()");

    QThread::currentThread()->setObjectName("SVDUI");

    modelThread->setObjectName("SVDMain");
    modelThread->start();

    dnnThread->setObjectName("SVDDNN");
    dnnThread->start();

}

ModelController::~ModelController()
{
    if (mModelShell)
        delete mModelShell;

}

void ModelController::setup(QString fileName)
{
    QMetaObject::invokeMethod(mModelShell, "setup", Qt::QueuedConnection,
                              Q_ARG(QString, fileName)) ;
}

void ModelController::shutdown()
{
    QMetaObject::invokeMethod(mModelShell, "abort", Qt::QueuedConnection);
}

void ModelController::run(int n_years)
{
    QMetaObject::invokeMethod(mModelShell, "run", Qt::QueuedConnection, Q_ARG(int,n_years));
}

