#ifndef MODELSHELL_H
#define MODELSHELL_H

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

#include "modelrunstate.h"
#include "spdlog/spdlog.h"
#include "toymodel.h"
//#include "core/model.h"



class ToyModelShell : public QObject
{
    Q_OBJECT
public:
    explicit ToyModelShell(QObject *parent = nullptr);
    ToyModel &toyModel() { return toy; }
    ~ToyModelShell();
signals:
    void log(const QString &s);
public slots:
    void run();
    void runTest();
    void process(int n);
    void abort();
private:
    bool mAbort;
    ToyModel toy;

};


class Model; // forward
class Cell; // forward
class InferenceData; // forward
class Batch; // forward


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

signals:
    void stateChanged(QString s);
    void processedStep(int n);
    void log(QString s);


    void newPackage(Batch *batch, int packageId);


public slots:
    void createModel(QString fileName);
    void setup();
    void runOneStep(int current_step);
    void run(int n_steps);
    void abort();
    //ModelRunState state() { return *mState; }

    void processedPackage(Batch *batch, int packageId);
    void allPackagesBuilt();

private:
    void internalRun();
    void buildInferenceData(Cell *cell);
    void checkBatch(Batch *batch);
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



    QFutureWatcher<void> packageWatcher;
    QFuture<void> packageFuture;

    // loggers
    std::shared_ptr<spdlog::logger> lg;



};

#endif // MODELSHELL_H
