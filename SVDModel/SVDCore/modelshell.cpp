#include "modelshell.h"
#include "toymodel.h"
#include "model.h"

#include "../Predictor/inferencedata.h"
#include "../Predictor/batchdnn.h"
#include "grid.h"
#include "outputs/outputmanager.h"

// needed only for visualization (to be removed again)
#include "modules/fire/firemodule.h"
#include "modules/module.h"

#include <QThread>
#include <QCoreApplication>
#include <QtConcurrent>

Q_DECLARE_METATYPE(Batch*)

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

    mModel = nullptr;
    mPackagesBuilt = 0;
    mPackageId = 0;

    qRegisterMetaType<Batch*>();


    connect(&packageWatcher, SIGNAL(finished()), this, SLOT(allPackagesBuilt()));
    //packageWatcher.setFuture(packageFuture);


}

ModelShell::~ModelShell()
{
    destroyModel();
}

void ModelShell::destroyModel()
{
    if (!mModel && Model::hasInstance())
        mModel = Model::instance(); // hackish way to make sure the global model is deleted

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

    if (what=="ext_seed") {
        auto &grid = Model::instance()->landscape()->grid();
        std::string result = gridToESRIRaster<Cell>(grid, [](const Cell &c)
        {
                return std::to_string(c.externalSeedType()); }
        );
        return result;
    }

    if (what=="fire_n") {
        FireModule *module = dynamic_cast<FireModule*>(Model::instance()->module("fire"));
        if (!module) return "fire module not active";
        const auto &grid = module->fireGrid();

        std::string result = gridToESRIRaster<SFireCell>(grid, [](const SFireCell &c)
        {
                return std::to_string(c.n_fire); }
        );
        return result;
    }

    if (what=="fire_year") {
        FireModule *module = dynamic_cast<FireModule*>(Model::instance()->module("fire"));
        if (!module) return "fire module not active";
        const auto &grid = module->fireGrid();

        std::string result = gridToESRIRaster<SFireCell>(grid, [](const SFireCell &c)
        {
                return std::to_string(c.last_burn); }
        );
        return result;
    }

    return "invalid method";
}


void ModelShell::createModel(QString fileName)
{
    try {
        setState(ModelRunState::Creating);
        if (Model::hasInstance())
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



static QMutex lock_processed_package;
void ModelShell::processedPackage(Batch *batch)
{
    mModel->stats.NPackagesDNN++;
    if (RunState::instance()->cancel() || batch->hasError()) {
        mPackagesProcessed++;
        lg->debug("error/cancel packages: all built: {}, #built: {}, #processed: {}, batch-id: {}", mAllPackagesBuilt, mPackagesBuilt, mPackagesProcessed, batch->packageId());
        if (mPackagesBuilt==mPackagesProcessed) {
            finalizeCycle();
        }
        return;
    }

    // TODO: this is a bit too much: some handling in derived batch types (DNN), some in modules (handlers)
    batch->processResults();

    if (batch->module()) {
        batch->module()->processBatch(batch);
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
        packageFuture = QtConcurrent::map(mModel->landscape()->grid(), [this](Cell &cell){ this->evaluateCell(&cell); });
        packageWatcher.setFuture(packageFuture);

        // run the modules
        mModel->runModules();

        // we can run the outputs conerning the current state right now (in parallel)
        mModel->outputManager()->run("StateGrid");
        mModel->outputManager()->run("ResTimeGrid");


}

void ModelShell::evaluateCell(Cell *cell)
{
    if (RunState::instance()->cancel())
        return;

    if (cell->isNull())
        return;
    if (cell->needsUpdate()==false)
        return;

    try {
        if( Module *module = cell->state()->module() ) {
            // extra module handles this state:
            // get a suitable slot in a batch for the given type
            std::pair<Batch*, size_t> slot = getSlot(cell, module);
            // invoke the module for preprocessing
            module->prepareCell(cell);
            // after processing, send the batch
            checkBatch(slot.first);
        } else {
            // default: Forest states and DNN
            buildInferenceDataDNN(cell);
        }

    } catch (const std::exception &e) {
        RunState::instance()->setError("Error: " + to_string(e.what()), RunState::instance()->modelState());
    }

}

// this function is executed for each cell on the landscape
// if a cell needs an update, the data is collected and the cell
// is stored within a batch.
void ModelShell::buildInferenceDataDNN(Cell *cell)
{

        assert(BatchManager::instance()!=nullptr);
        std::pair<Batch*, size_t> newslot = BatchManager::instance()->validSlot(nullptr);
        BatchDNN *batch = dynamic_cast<BatchDNN*>(newslot.first);
        if (!batch) {
            if (newslot.second==0)
                return; // the operation has been canceled while waiting for a slot above
            else {
                // report an error
                RunState::instance()->setError("Error: cannot find a valid slot. Timeout?", RunState::instance()->modelState());
                return;
            }
        }
        InferenceData &id = batch->inferenceData(newslot.second);

        // populate the InferenceData with the required data
        id.fetchData(cell, batch, newslot.second);

        // check if batch is finished and send if this is the case
        if (checkBatch(batch))
            lg->debug("sent package [{}] when processing slot {} (used slots: {})", static_cast<void*>(batch), newslot.second, batch->usedSlots());


}

std::pair<Batch*, size_t> ModelShell::getSlot(Cell *cell, Module *module)
{
    assert(BatchManager::instance()!=nullptr);
    std::pair<Batch*, size_t> newslot = BatchManager::instance()->validSlot(module);
    Batch *batch = newslot.first;
    if (!batch) {
        if (newslot.second==0)
            return newslot; // the operation has been canceled while waiting for a slot above
        else {
            // report an error
            RunState::instance()->setError("Error: cannot find a valid slot. Timeout?", RunState::instance()->modelState());
            return newslot;
        }
    }
    batch->setCell(cell, newslot.second);
    return newslot;

}


static QMutex _check_batch;
/// sends a package to the Inference process when all slots are filled.
bool ModelShell::checkBatch(Batch *batch)
{
    if (RunState::instance()->cancel()) {
        cancel();
        return false;
    }

    QMutexLocker lock(&_check_batch);
    batch->finishedCellProcessing();
    if (batch->allCellsProcessed()) {
        if (batch->state()!=Batch::Fill) {
            lg->warn("Package [{}] already sent!", static_cast<void*>(batch));
            return false;
        }

        // send batch to inference (DNN or other threads)
        sendBatch(batch);
        return true;

    }
    return false;
}

void ModelShell::sendBatch(Batch *batch)
{
    batch->setPackageId(++mPackageId);
    mModel->stats.NPackagesSent ++;
    ++mPackagesBuilt;
    // send via signal/slot for DNN packages
    if (batch->type()==Batch::DNN) {
        lg->debug("sending package {} [{}] to Inference (built total: {})", mPackageId, static_cast<void*>(batch), mPackagesBuilt);
        emit newPackage(batch);
    } else {
        // directly call the function (in a thread)
        QtConcurrent::run([](ModelShell *shell, Batch *batch) {
            // do some work!! module->run(batch) ??
            if (!QMetaObject::invokeMethod(shell, "processedPackage", Qt::QueuedConnection,
                                           Q_ARG(Batch*, batch)) ) {
                batch->setError(true);
            }

        }, this, batch);
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
            sendBatch(e);
            lg->debug("sending pending package {} [{}] to Inference. (total. {}, size queue: {})", mPackageId, static_cast<void*>(e), mPackagesBuilt, BatchManager::instance()->batches().size());

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


