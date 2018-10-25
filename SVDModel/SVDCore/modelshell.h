#ifndef MODELSHELL_H
#define MODELSHELL_H

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

#include "modelrunstate.h"
#include "spdlog/spdlog.h"


class Model; // forward
class Cell; // forward
class InferenceData; // forward
class Batch; // forward
class Module; // forward


class ModelShell: public QObject
{
    Q_OBJECT
    Q_ENUMS(ModelRunState)
public:
    explicit ModelShell(QObject *parent = nullptr);
    ~ModelShell();
    void destroyModel();

    Model *model() { return mModel; }

    // test function
    std::string run_test_op(std::string what);
    int packagesBuilt() const { return mPackagesBuilt; }
    int packagesProcessed() const { return mPackagesProcessed; }
    size_t cellsProcessed() const { return mCellsProcesssed; }

signals:
    void stateChanged(QString s);
    void processedStep(int n);
    void log(QString s);


    void newPackage(Batch *batch);


public slots:
    void createModel(QString fileName);
    void setup();
    void runOneStep(int current_step);
    void run(int n_steps);
    void abort();
    //ModelRunState state() { return *mState; }

    void processedPackage(Batch *batch);
    void allPackagesBuilt();

private:
    void internalRun();
    void evaluateCell(Cell *cell);
    void buildInferenceDataDNN(Cell *cell);
    std::pair<Batch *, size_t> getSlot(Cell *cell, Module *module);
    bool checkBatch(Batch *batch);
    void sendBatch(Batch *batch);
    void sendPendingBatches();
    void finalizeCycle();
    void cancel();

    void processEvents();


    void setState(ModelRunState::State new_state, QString msg=QString());
    bool mAbort;
    Model *mModel;

    int mPackagesBuilt;
    int mPackagesProcessed;
    bool mAllPackagesBuilt;
    int mPackageId;
    size_t mCellsProcesssed; // cells that are processed in the model (not via DNN)



    QFutureWatcher<void> packageWatcher;
    QFuture<void> packageFuture;

    // loggers
    std::shared_ptr<spdlog::logger> lg;



};

#endif // MODELSHELL_H
