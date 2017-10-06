#include "modelshell.h"
#include "toymodel.h"
#include "model.h"

#include "../Predictor/inferencedata.h"
#include "grid.h"

#include <QThread>
#include <QCoreApplication>
#include <QtConcurrent>


ToyModelShell::ToyModelShell(QObject *parent) : QObject(parent)
{
    mAbort = false;
    emit log("Model shell created.");
}

ToyModelShell::~ToyModelShell()
{
    emit log("Model shell destroyed");
}

void ToyModelShell::run()
{
    emit log(QString("Running thread %1").arg(QThread::currentThread()->objectName()));

    toy.run();

    emit log("... model running...");
}

void ToyModelShell::runTest()
{
    emit log("Running....");
    emit log(QString("%1").arg(QThread::currentThread()->objectName()));
    //QThread::sleep(3);

    for (int i=0;i<10;++i) {

        QCoreApplication::processEvents();
        if (mAbort)
            break;

        QThread::sleep(1);
        emit log(QString::number(i));
    }

    emit log("Finished run");

}

void ToyModelShell::process(int n)
{
    for (int i=0;i<n;++i)
        emit log(QString("iteration %1").arg(i));
}

void ToyModelShell::abort()
{
    mAbort = true;
    toy.abort();
}

// ***************** ModelShell ********************************

ModelShell::ModelShell(QObject *parent)
{
    Q_UNUSED(parent);
    mAbort = false;
    mState = Invalid;
    mModel = 0;
    mLivePackages = 0;
    mPackageId = 0;

    connect(&packageWatcher, SIGNAL(finished()), this, SLOT(allPackagesBuilt()));
    //packageWatcher.setFuture(packageFuture);


}

ModelShell::~ModelShell()
{
    destroyModel();
}

void ModelShell::destroyModel()
{
    if (mModel) {
        lg.reset(); // delete link to the logging stream
        Model *m = mModel;
        mModel = nullptr;
        delete m;
    }
}

bool ModelShell::isModelRunning() const
{
    return (mState == Creating || mState==Stopping || mState==Running);
}

std::string ModelShell::run_test_op(std::string what)
{
    if (what=="grid_state") {
        auto &grid = Model::instance()->landscape()->currentGrid();
        std::string result = gridToESRIRaster<Cell>(grid, [](const Cell &c) { if (c.isNull()) return std::string("-9999"); else return std::to_string(c.state()); });
        return result;
    }
    if (what=="grid_restime") {
        auto &grid = Model::instance()->landscape()->currentGrid();
        std::string result = gridToESRIRaster<Cell>(grid, [](const Cell &c)
        { if (c.isNull())
                return std::string("-9999");
            else
                return std::to_string(c.residenceTime()); }
        );
        return result;
    }
    if (what=="grid_next") {
        auto &grid = Model::instance()->landscape()->currentGrid();
        std::string result = gridToESRIRaster<Cell>(grid, [](const Cell &c)
        { if (c.isNull())
                return std::string("-9999");
            else
                return std::to_string(c.nextUpdate()); }
        );
        return result;
    }

    if (what=="grid_N") {
        auto &grid = Model::instance()->landscape()->environment();
        std::string result = gridToESRIRaster<EnvironmentCell*>(grid, [](EnvironmentCell *c)
        { if (!c)
                return std::string("-9999");
            else
                return std::to_string(c->value("availableNitrogen")); }
        );
        return result;
    }


    return "invalid method";
}
void ModelShell::createModel(QString fileName)
{
    try {
        setState(Creating);
        if (model())
            destroyModel();

        mModel = new Model(fileName.toStdString());

    } catch (const std::exception &e) {
        if (spdlog::get("main"))
            spdlog::get("main")->error("An error occured: {}", e.what());
        setState( ErrorDuringSetup, QString(e.what()));
    }
}

void ModelShell::setup()
{
    try {
        if (!model())
            throw std::logic_error("ModelShell::setup: model is NULL");

        setState(Creating);

        if (model()->setup(  )) {
            // setup successful
            lg = spdlog::get("main");
            lg->info("Model successfully set up in Thread {}.", QThread::currentThread()->objectName().toStdString());

            setState(ReadyToRun);
        }


    } catch (const std::exception &e) {
        if (spdlog::get("main"))
            spdlog::get("main")->error("An error occured: {}", e.what());
        setState( ErrorDuringSetup, QString(e.what()));
    }
}

void ModelShell::runOneStep()
{
    setState(Running);
    try {
        spdlog::get("main")->info("Run one step.");
        // run the model...
        internalRun();



    } catch (const std::exception &e) {
        spdlog::get("main")->error("An error occured: {}", e.what());
        setState( Error, QString(e.what()));
    }
}

void ModelShell::run(int n_steps)
{

    runOneStep();
    return;

    setState(Running);
    try {
        spdlog::get("main")->info("Run {} steps.", n_steps);
        // run the model...
        for (int i=0;i<n_steps;++i) {
            QCoreApplication::processEvents();
            spdlog::get("main")->info("Run step {} of {}.", i+1, n_steps);
            setState(Running, QString("year %1 of %2.").arg(i+1).arg(n_steps));
            if (mAbort) {
                setState(Canceled);
                return;
            }

            internalRun();


        }
        setState(ReadyToRun);


    } catch (const std::exception &e) {
        spdlog::get("main")->error("An error occured: {}", e.what());
        setState( Error, QString(e.what()));
    }

}

void ModelShell::abort()
{
    mAbort = true;
    if (mState==Creating || mState==Running) {
        setState(Stopping, "by user request.");
    } else {
        setState(Invalid, "model destroyed");
        destroyModel();
    }
}

QString ModelShell::stateString(ModelRunState s)
{
    // Invalid=0, Creating, ReadyToRun, Stopping, Running, Paused, Finsihed, Canceled, ErrorDuringSetup, Error };
    switch (s) {
    case Invalid: return QStringLiteral("Invalid");
    case Creating: return QStringLiteral("Creating...");
    case ReadyToRun: return QStringLiteral("Ready.");
    case Stopping: return QStringLiteral("Stopping...");
    case Running: return QStringLiteral("Running...");
    case Finsihed: return QStringLiteral("Finished!");
    case Canceled: return QStringLiteral("Canceled!");
    case ErrorDuringSetup: return QStringLiteral("Error during setup!!");
    case Error: return QStringLiteral("Error!!");
    default: return QStringLiteral("invalid state");
    }

}

void ModelShell::setState(ModelShell::ModelRunState new_state, QString msg)
{
    mState = new_state;
    QString text = stateString(mState);
    if (msg.isEmpty())
        emit stateChanged(text);
    else
        emit stateChanged(text + " - " + msg);
}



QMutex lock_processed_package;
void ModelShell::processedPackage(std::list<InferenceData *> *package, int packageId)
{
    // DNN delivered processed package....
    lg->debug("Model: DNN package {} received!", packageId);

    // write back to data.....
    for (InferenceData * ii : *package) {
        ii->writeResult();
    }


    // now the data can be freed:
    {
    QMutexLocker locker(&lock_processed_package);
    mModel->stats.NPackagesDNN += package->size();

    mLivePackages--;

    for (auto p : *package)
        delete p;

    delete package;
    }

    if (mLivePackages==0) {
        lg->info( "Model: processsed Last Package! [NSent: {} NReceived: {}]", mModel->stats.NPackagesSent, mModel->stats.NPackagesDNN );
        finalizeCycle();
    } else {
        lg->debug( "Model: #packages: {} [NSent: {} NReceived: {}]", mLivePackages, mModel->stats.NPackagesSent, mModel->stats.NPackagesDNN);
    }

}

void ModelShell::allPackagesBuilt()
{
    //lg->debug("** all packages built, starting the last package");
    sendInferencePackage(); // start last batch (even if < than batch size)
}

void ModelShell::internalRun()
{

    try {
        // increment the time step of the model
        mModel->newYear();

        // check for each cell if we need to do something; if yes, then
        // create a InferenceData item (and eventually add it to a 'package')
        packageFuture = QtConcurrent::map(mModel->landscape()->currentGrid(), [this](Cell &cell){ this->buildInferenceData(&cell); });
        packageWatcher.setFuture(packageFuture);

    } catch (const ThreadSafeException &e) {
        // if an exception occures during the execution of the QtConcurrent function, then a thread safe exception is created,
        // and catched here in the main thread.
        lg->error("An error occured: {}", e.what());
        throw std::logic_error(e.what());
    }

}

void ModelShell::buildInferenceData(Cell *cell)
{
    try {
        // this function is called from multiple threads....
        if (cell->isNull())
            return;
        if (cell->needsUpdate()==false)
            return;

        // create an InferenceData item and populate it with the required data
        // the c'tor requests all the information from the model
        InferenceData *item = new InferenceData(cell);

        // add to processing chain
        addDataPackage(item);

    } catch (const std::exception &e) {
        lg->error("An error occured: {}", e.what());
        throw ThreadSafeException(QString(e.what()));
    }

}


QMutex lock_inference_list;
/// addDataPackage adds the data package 'item' to
/// the internal list. If the batch_size is reached,
/// a package is sent to the DNN thread.
void ModelShell::addDataPackage(InferenceData *item)
{
    const int batch_size = 128;
    QMutexLocker locker(&lock_inference_list);
    if (mInferenceData.size()==0) {
        mInferenceData.push_back( new std::list<InferenceData*>() );
        mLivePackages++;
    }

    std::list<InferenceData*> *list = mInferenceData.back();
    if (list->size()>=batch_size) {
        sendInferencePackage();

        mInferenceData.push_back( new std::list<InferenceData*>() );
        mLivePackages++;
        list = mInferenceData.back();
        // std::cout << "created new data list, #=" << for_inference.size() << std::endl;
    }
    list->push_back(item);

}


/// sendInferencePackage() takes the next batch of items and emits a signal
/// to the DNN thread that a new batch is ready to process
/// Note that this code is still protected by the 'lock_inference_list' mutex
void ModelShell::sendInferencePackage()
{
    if (mInferenceData.size()==0)
        return;

    // take the first element of the list:
    std::list<InferenceData*> *inf_list = mInferenceData.front();
    mInferenceData.pop_front();

    // Call inference machine: route to DNN thread
    while (mLivePackages>3 && mAbort==false) {
        // wait some time...
        lg->debug("... wait until <=3 .... #packages= {}", mLivePackages);
        QThread::msleep(1000);
        QCoreApplication::processEvents(); // allow receiving of packages...

    }
    mPackageId++;
    mModel->stats.NPackagesSent += inf_list->size();
    lg->debug("sending package to Inference id={} (now in queue {})", mPackageId, mLivePackages);
    emit newPackage(inf_list, mPackageId);

}

void ModelShell::finalizeCycle()
{
    // everything is
    mModel->finalizeYear();
    lg->info("Year {} finished.", mModel->year());

    setState(ReadyToRun);
}


