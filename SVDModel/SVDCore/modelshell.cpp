#include "modelshell.h"
#include "toymodel.h"
#include "model.h"

#include "../Predictor/inferencedata.h"
#include "../Predictor/batch.h"
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
    mState = ModelRunState::Invalid;
    mModel = 0;
    mPackagesBuilt = 0;
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
    return (mState == ModelRunState::Creating || mState==ModelRunState::Stopping || mState==ModelRunState::Running);
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

void ModelShell::dnnState(QString msg)
{
    // a message from DNN
    if (spdlog::get("main"))
        spdlog::get("main")->debug("A message from the DNN: {}", msg.toStdString());
    if (msg=="startup.ok")
        mDNNState = ModelRunState::ReadyToRun;
    if (msg=="startup.error")
        mDNNState = ModelRunState::ErrorDuringSetup;
    if (msg=="error")
        mDNNState = ModelRunState::Error;

}
void ModelShell::createModel(QString fileName)
{
    try {
        mDNNState.set(ModelRunState::Creating);
        setState(ModelRunState::Creating);
        mDNNState = ModelRunState::Creating;
        if (model())
            destroyModel();

        mModel = new Model(fileName.toStdString());
        mModel->setProcessEventsCallback( std::bind(&ModelShell::processEvents, this) );

    } catch (const std::exception &e) {
        if (spdlog::get("main"))
            spdlog::get("main")->error("An error occured: {}", e.what());
        setState( ModelRunState::ErrorDuringSetup, QString(e.what()));
    }
}

void ModelShell::setup()
{
    try {
        if (!model())
            throw std::logic_error("ModelShell::setup: model is NULL");

        setState(ModelRunState::Creating);

        if (model()->setup(  )) {
            // setup successful
            lg = spdlog::get("main");
            while (mDNNState == ModelRunState::Creating) {
                lg->debug("waiting for DNN thread...");
                QThread::msleep(50);
                QCoreApplication::processEvents();
            }
            if (mDNNState==ModelRunState::ReadyToRun) {
                lg->info("Model successfully set up in Thread {}.", QThread::currentThread()->objectName().toStdString());
                setState(ModelRunState::ReadyToRun);
            } else {
                lg->info("Model setup, but error in setup of DNN.");
                setState(ModelRunState::ErrorDuringSetup);
            }
        }


    } catch (const std::exception &e) {
        if (spdlog::get("main"))
            spdlog::get("main")->error("An error occured: {}", e.what());
        setState( ModelRunState::ErrorDuringSetup, QString(e.what()));
    }
}

void ModelShell::runOneStep()
{
    setState(ModelRunState::Running);
    try {
        spdlog::get("main")->info("Run one step.");
        // run the model...
        internalRun();



    } catch (const std::exception &e) {
        spdlog::get("main")->error("An error occured: {}", e.what());
        setState( ModelRunState::Error, QString(e.what()));
    }
}

void ModelShell::run(int n_steps)
{

    runOneStep();
    return;

    setState(ModelRunState::Running);
    try {
        spdlog::get("main")->info("Run {} steps.", n_steps);
        // run the model...
        for (int i=0;i<n_steps;++i) {
            QCoreApplication::processEvents();
            spdlog::get("main")->info("Run step {} of {}.", i+1, n_steps);
            setState(ModelRunState::Running, QString("year %1 of %2.").arg(i+1).arg(n_steps));
            if (mAbort) {
                setState(ModelRunState::Canceled);
                return;
            }

            internalRun();


        }
        setState(ModelRunState::ReadyToRun);


    } catch (const std::exception &e) {
        spdlog::get("main")->error("An error occured: {}", e.what());
        setState( ModelRunState::Error, QString(e.what()));
    }

}

void ModelShell::abort()
{
    mAbort = true;
    if (mState==ModelRunState::Creating || mState==ModelRunState::Running) {
        setState(ModelRunState::Stopping, "by user request.");
    } else {
        setState(ModelRunState::Invalid, "model destroyed");
        destroyModel();
    }
}



void ModelShell::setState(ModelRunState::State new_state, QString msg)
{
    mState = new_state;
    QString text = QString::fromStdString(mState.stateString());
    if (msg.isEmpty())
        emit stateChanged(text);
    else
        emit stateChanged(text + " - " + msg);
}



QMutex lock_processed_package;
void ModelShell::processedPackage(Batch *batch, int packageId)
{
    if (batch->hasError()) {
        return;
    }


    // DNN delivered processed package....
    lg->debug("Model: DNN package {} received!", packageId);

    for (int i=0;i<batch->usedSlots();++i) {
        batch->inferenceData(i).writeResult();
    }
    batch->changeState(Batch::Fill);



    // now the data can be freed:
    {
        QMutexLocker locker(&lock_processed_package);
        mPackagesProcessed++;

    }

    if (mAllPackagesBuilt && mPackagesBuilt==mPackagesProcessed) {
        lg->info( "Model: processsed Last Package! [NSent: {} NReceived: {}]", mModel->stats.NPackagesSent, mModel->stats.NPackagesDNN );
        finalizeCycle();
    } else {
        lg->debug( "Model: #packages: {} sent, {} processed [total: NSent: {} NReceived: {}]", mPackagesBuilt, mPackagesProcessed, mModel->stats.NPackagesSent, mModel->stats.NPackagesDNN);
    }

}

void ModelShell::allPackagesBuilt()
{
    //lg->debug("** all packages built, starting the last package");
    sendPendingBatches(); // start last batch (even if < than batch size)
    mAllPackagesBuilt = true;
}

void ModelShell::internalRun()
{

    try {
        // increment the time step of the model
        mModel->newYear();
        mAllPackagesBuilt=false;
        mPackagesBuilt=0;
        mPackagesProcessed=0;

        // check for each cell if we need to do something; if yes, then
        // fill a InferenceData item within a batch of data
        //
        packageFuture = QtConcurrent::map(mModel->landscape()->currentGrid(), [this](Cell &cell){ this->buildInferenceData(&cell); });
        packageWatcher.setFuture(packageFuture);
        packageFuture.waitForFinished(); // TODO: this causes hanging ... if we don't wait, we don't get exceptions!

    } catch (const ThreadSafeException &e) {
        // if an exception occures during the execution of the QtConcurrent function, then a thread safe exception is created,
        // and catched here in the main thread.
        lg->error("An error occured: {}", e.what());
        setState(ModelRunState::Error);
        throw std::logic_error(e.what());
    }

}

void ModelShell::buildInferenceData(Cell *cell)
{
    if (mDNNState==ModelRunState::Error)
        return;

    try {
        // this function is called from multiple threads....
        if (cell->isNull())
            return;
        if (cell->needsUpdate()==false)
            return;

        assert(BatchManager::instance()!=nullptr);
        std::pair<Batch*, int> newslot = BatchManager::instance()->validSlot();
        if (!newslot.first)
            throw std::logic_error("Error: cannot find a valid slot. Timeout?");
        InferenceData &id = newslot.first->inferenceData(newslot.second);

        // populate the InferenceData with the required data
        id.fetchData(cell, newslot.first, newslot.second);

        checkBatch(newslot.first);


    } catch (const std::exception &e) {
        lg->error("An error occured: {}", e.what());
        throw ThreadSafeException(e);
    }

}




void ModelShell::checkBatch(Batch *batch)
{
    if (mDNNState == ModelRunState::Error) {
        cancel();
        return;
    }
    if (batch->freeSlots()<=0) {
        mPackageId++;
        mModel->stats.NPackagesSent ++;
        ++mPackagesBuilt;
        lg->debug("sending package to Inference id={} (now in queue {})", mPackageId, mPackagesBuilt);
        emit newPackage(batch, mPackageId);

    }
}


/// sendInferencePackage() takes the next batch of items and emits a signal
/// to the DNN thread that a new batch is ready to process
/// Note that this code is still protected by the 'lock_inference_list' mutex
void ModelShell::sendPendingBatches()
{
    if (state()==ModelRunState::Error)
        return;

    for (auto e : BatchManager::instance()->batches()) {
        if (e->state()==Batch::Fill && e->usedSlots()>0) {
            mPackageId++;
            mModel->stats.NPackagesSent ++;
            ++mPackagesBuilt;
            lg->debug("sending pending package to Inference id={} (now in queue {}, Batch manager: {})", mPackageId, mPackagesBuilt, BatchManager::instance()->batches().size());
            emit newPackage(e, mPackageId);
        }
    }
}

void ModelShell::finalizeCycle()
{
    // everything is
    mModel->finalizeYear();
    lg->info("Year {} finished.", mModel->year());

    setState(ModelRunState::ReadyToRun);
}

void ModelShell::cancel()
{
    lg->error("An error occured - stopping.");
    setState(ModelRunState::Error);
}

void ModelShell::processEvents()
{
    QCoreApplication::processEvents();
}


