#include "modelcontroller.h"
#include "modelshell.h"
#include "toymodel.h"
#include "model.h"
#include "../Predictor/batchmanager.h"
#include "../Predictor/batch.h"

#include "../Predictor/dnnshell.h"

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


ModelController::ModelController(QObject *)
{
    mState = std::unique_ptr<RunState>(new RunState());

    ModelShell *model_shell = new ModelShell;
    modelThread = new QThread();
    mModelShell = model_shell;
    mModelShell->moveToThread(modelThread);

    connect(modelThread, &QThread::finished, model_shell, &QObject::deleteLater);
    connect(mModelShell, &ModelShell::stateChanged, this, &ModelController::stateChanged);


    dnnThread = new QThread();
    mDNNShell = new DNNShell;
    mDNNShell->moveToThread(dnnThread);

    connect(dnnThread, &QThread::finished, mDNNShell, &QObject::deleteLater);

    // connection between main model and DNN: [this requires the old way of connect, because there are errors with Batch* otherwise]
    QObject::connect(mModelShell, SIGNAL(newPackage(Batch*)), mDNNShell, SLOT(doWork(Batch*)), Qt::QueuedConnection);
    QObject::connect(mDNNShell, SIGNAL(workDone(Batch*)), mModelShell, SLOT(processedPackage(Batch*)), Qt::QueuedConnection);
    //connect(mModelShell, &ModelShell::newPackage, mModelInterface, &ModelInterface::doWork);
    //connect(mModelInterface, &ModelInterface::workDone, mModelShell, &ModelShell::processedPackage);
    //connect(mModelShell, &ModelShell::finished, this, &ModelController::finishedRun, Qt::QueuedConnection);

    // fired after a simulation year ended
    connect(mModelShell, &ModelShell::processedStep, this, &ModelController::finishedStep, Qt::QueuedConnection);

    // logging
    connect(mModelShell, &ModelShell::log, this, &ModelController::log, Qt::QueuedConnection);

    QThread::currentThread()->setObjectName("SVDUI");

    modelThread->setObjectName("SVDMain");
    modelThread->start();

    dnnThread->setObjectName("SVDDNN");
    dnnThread->start();

    mYearsToRun = 0;
    mCurrentStep = 0;

}

ModelController::~ModelController()
{
    shutdown();
    // shut down threads...
    modelThread->quit();
    dnnThread->quit();
    modelThread->wait();
    dnnThread->wait();

    //if (mModelShell)
    //    delete mModelShell;
    //if (mDNNShell)
    //    delete mDNNShell;

}

std::unordered_map<std::string, std::string> ModelController::systemStatus()
{
    std::unordered_map<std::string, std::string> result;
    if (mModelShell->model() == nullptr)
        return result;

    // add statistics
    result["dnnRunning"] = dnnShell()->isRunnig() ? "yes" : "no";
    result["dnnBatchesProcessed"] = to_string(dnnShell()->batchesProcessed());
    result["dnnCellsProcessed"] = to_string(dnnShell()->cellsProcessed());
    result["cellsPerSecond"] = to_string(dnnShell()->cellsProcessed() / (mStopWatch.elapsed()>0 ? mStopWatch.elapsed()/1000. : 1.));

    int n_fill=0, n_finished=0, n_dnn=0;
    int n_open_slots = 0;
    for (auto e : BatchManager::instance()->batches()) {
        switch (e->state()) {
        case Batch::Fill: ++n_fill; n_open_slots += e->freeSlots();  break;
        case Batch::Finished: ++n_finished; break;
        case Batch::DNN: ++n_dnn; break;
        }
    }
    result["batches"] = to_string( BatchManager::instance()->batches().size() );
    result["batchesFinished"] = to_string(n_finished);
    result["batchesDNN"] = to_string(n_dnn);
    result["batchesAvailable"] = to_string(n_fill);
    result["batchCellAvailable"] = to_string(n_open_slots);

    // main packages...
    result["mainBatchesBuilt"] = to_string( shell()->packagesBuilt() );
    result["mainBatchesProcessed"] = to_string( shell()->packagesProcessed() );

    return result;
}

void ModelController::setup(QString fileName)
{
    // use a blocking connection for initial creation (logging, etc.)
    QMetaObject::invokeMethod(mModelShell, "createModel", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, fileName)) ;

    if (RunState::instance()->isError())
        return;


    QMetaObject::invokeMethod(mModelShell, "setup", Qt::QueuedConnection) ;
    if (RunState::instance()->isError())
        return;


    QMetaObject::invokeMethod(mDNNShell, "setup", Qt::QueuedConnection,
                              Q_ARG(QString, fileName)) ;

}

void ModelController::shutdown()
{
    RunState::instance()->setCancel( true ); // stop running operations
    QMetaObject::invokeMethod(mModelShell, "abort", Qt::QueuedConnection);
}

void ModelController::run(int n_years)
{
    mYearsToRun = n_years;
    mCurrentStep = 1;
    mStopWatch.start();
    QMetaObject::invokeMethod(mModelShell, "run", Qt::QueuedConnection, Q_ARG(int,n_years));
}


void ModelController::finishedStep(int n)
{
    mCurrentStep++;
    if (mCurrentStep >= mYearsToRun) {
        // finished
        log(QString("Finished!"));
        RunState::instance()->modelState()=ModelRunState::Finished;
        emit finished();
        return;
    } else {
        emit finishedYear(mCurrentStep);
    }
    // run the next year of the simulation
    QMetaObject::invokeMethod(mModelShell, "runOneStep", Qt::QueuedConnection, Q_ARG(int, mCurrentStep));
    log(QString("finished %1 of %2.").arg(n).arg(mYearsToRun));

}

