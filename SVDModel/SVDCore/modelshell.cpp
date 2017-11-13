#include "modelshell.h"
#include "toymodel.h"
#include "model.h"

#include "../Predictor/inferencedata.h"
#include "../Predictor/batch.h"
#include "grid.h"
#include "outputs/outputmanager.h"

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


std::string ModelShell::run_test_op(std::string what)
{
    if (what=="grid_state") {
        auto &grid = Model::instance()->landscape()->grid();
        std::string result = gridToESRIRaster<Cell>(grid, [](const Cell &c) { if (c.isNull()) return std::string("-9999"); else return std::to_string(c.stateId()); });
        return result;
    }
    if (what=="grid_restime") {
        auto &grid = Model::instance()->landscape()->grid();
        std::string result = gridToESRIRaster<Cell>(grid, [](const Cell &c)
        { if (c.isNull())
                return std::string("-9999");
            else
                return std::to_string(c.residenceTime()); }
        );
        return result;
    }
    if (what=="grid_next") {
        auto &grid = Model::instance()->landscape()->grid();
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
        setState(ModelRunState::Creating);
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
            while (RunState::instance()->dnnState() == ModelRunState::Creating) {
                lg->debug("waiting for DNN thread...");
                QThread::msleep(50);
                QCoreApplication::processEvents();
            }
            setState( ModelRunState::ReadyToRun );
        }


    } catch (const std::exception &e) {
        if (spdlog::get("main"))
            spdlog::get("main")->error("An error occured: {}", e.what());
        setState( ModelRunState::ErrorDuringSetup, QString(e.what()));
    }
}

void ModelShell::runOneStep(int current_step)
{
    setState(ModelRunState::Running);
    try {
        spdlog::get("main")->info("Run year {}.", current_step);
        spdlog::get("main")->info("*****************************");

        // run the model...
        internalRun();



    } catch (const std::exception &e) {
        spdlog::get("main")->error("An error occured: {}", e.what());
        setState( ModelRunState::Error, QString(e.what()));
    }
}

void ModelShell::run(int n_steps)
{

    //runOneStep();
    //return;
    spdlog::get("main")->info("***********************************************");
    spdlog::get("main")->info("Start the simulation of {} steps.",n_steps);

    runOneStep(1);

}

void ModelShell::abort()
{
    mAbort = true;
    /*
    if (mState==ModelRunState::Creating || mState==ModelRunState::Running) {
        setState(ModelRunState::Stopping, "by user request.");
    } else {
        setState(ModelRunState::Invalid, "model destroyed");
        destroyModel();
    } */
}



void ModelShell::setState(ModelRunState::State new_state, QString msg)
{
    RunState::instance()->modelState().set(new_state);

    QString text = QString::fromStdString(RunState::instance()->asString());
    if (msg.isEmpty())
        emit stateChanged(text);
    else
        emit stateChanged(text + " - " + msg);
}



QMutex lock_processed_package;
void ModelShell::processedPackage(Batch *batch, int packageId)
{
    if (RunState::instance()->cancel() || batch->hasError()) {
        mPackagesProcessed++;
        lg->debug("error/cancel packages: all built: {}, #built: {}, #processed: {}, batch-id: {}", mAllPackagesBuilt, mPackagesBuilt, mPackagesProcessed, packageId);
        if (mPackagesBuilt==mPackagesProcessed) {
            finalizeCycle();
        }
        return;
    }


    // DNN delivered processed package....
    lg->debug("Model: received package {} (from DNN). Writing back data.", packageId);

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
    if (!BatchManager::instance()->slotsRequested()) {
        lg->debug("No pixel was updated this year.");
        finalizeCycle();
    }
    //lg->debug("** all packages built, starting the last package");
    sendPendingBatches(); // start last batch (even if < than batch size)
    mAllPackagesBuilt = true;
}

void ModelShell::internalRun()
{


        // increment the time step of the model
        mModel->newYear();
        mAllPackagesBuilt=false;
        mPackagesBuilt=0;
        mPackagesProcessed=0;

        // check for each cell if we need to do something; if yes, then
        // fill a InferenceData item within a batch of data
        //
        packageFuture = QtConcurrent::map(mModel->landscape()->grid(), [this](Cell &cell){ this->buildInferenceData(&cell); });
        packageWatcher.setFuture(packageFuture);

        // we can run the outputs conerning the current state right now (in parallel)
        mModel->outputManager()->run("StateGrid");
        mModel->outputManager()->run("ResTimeGrid");


}

void ModelShell::buildInferenceData(Cell *cell)
{
    if (RunState::instance()->cancel())
        return;

    try {
        // this function is called from multiple threads....
        if (cell->isNull())
            return;
        if (cell->needsUpdate()==false)
            return;

        assert(BatchManager::instance()!=nullptr);
        std::pair<Batch*, int> newslot = BatchManager::instance()->validSlot();
        Batch *batch = newslot.first;
        if (!batch) {
            if (newslot.second==0)
                return; // cancel operation
            else {
                // report an error
                RunState::instance()->setError("Error: cannot find a valid slot. Timeout?", RunState::instance()->modelState());
                return;
            }
        }
        InferenceData &id = batch->inferenceData(newslot.second);

        // populate the InferenceData with the required data
        id.fetchData(cell, batch, newslot.second);

        batch->finishedCellProcessing();

        checkBatch(batch);


    } catch (const std::exception &e) {
        RunState::instance()->setError("Error: " + to_string(e.what()), RunState::instance()->modelState());
    }

}




void ModelShell::checkBatch(Batch *batch)
{
    if (RunState::instance()->cancel()) {
        cancel();
        return;
    }
    if (batch->allCellsProcessed()) {
        mPackageId++;
        mModel->stats.NPackagesSent ++;
        ++mPackagesBuilt;
        lg->debug("sending package {} to Inference (built total: {})", mPackageId, mPackagesBuilt);
        emit newPackage(batch, mPackageId);

    }
}


/// sendInferencePackage() takes the next batch of items and emits a signal
/// to the DNN thread that a new batch is ready to process
/// Note that this code is still protected by the 'lock_inference_list' mutex
void ModelShell::sendPendingBatches()
{
    if (RunState::instance()->cancel())
        return;

    for (auto e : BatchManager::instance()->batches()) {
        if (e->state()==Batch::Fill && e->usedSlots()>0) {
            mPackageId++;
            mModel->stats.NPackagesSent ++;
            ++mPackagesBuilt;
            lg->debug("sending pending package {} to Inference. (total. {}, size queue: {})", mPackageId, mPackagesBuilt, BatchManager::instance()->batches().size());
            emit newPackage(e, mPackageId);
        }
    }
}

void ModelShell::finalizeCycle()
{
    if (RunState::instance()->cancel()) {
        lg->info("Stopped in year {}.", mModel->year());
        setState(ModelRunState::Canceled);
        return;
    }
    // everything is
    mModel->finalizeYear();
    lg->info("Year {} finished.", mModel->year());

    setState(ModelRunState::ReadyToRun);
    emit processedStep(mModel->year());
}

void ModelShell::cancel()
{
    lg->error("An error occured - stopping.");
    setState(ModelRunState::Stopping);
}

void ModelShell::processEvents()
{
    QCoreApplication::processEvents();
}


