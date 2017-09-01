#ifndef MODELSHELL_H
#define MODELSHELL_H

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

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


class ThreadSafeException : public QtConcurrent::Exception
{
public:
    ThreadSafeException(QString msg) { mMsg = msg; }
    char const * what() const {return mMsg.toStdString().c_str(); }
    void raise() const { throw *this; }
    ThreadSafeException *clone() const { return new ThreadSafeException(*this); }
private:
    QString mMsg;
};

class Model; // forward
class Cell;
class InferenceData;


class ModelShell: public QObject
{
    Q_OBJECT
    Q_ENUMS(ModelRunState)
public:
    enum ModelRunState { Invalid=0, Creating, ReadyToRun, Stopping, Running, Paused, Finsihed, Canceled, ErrorDuringSetup, Error };
    explicit ModelShell(QObject *parent = nullptr);
    ~ModelShell();
    void destroyModel();

    Model *model() { return mModel; }

    bool isModelRunning() const;

    // test function
    std::string run_test_op(std::string what);

signals:
    void stateChanged(QString s);
    void processedStep(int n);
    void log(QString s);


    void newPackage(std::list<InferenceData*>*, int packageId);
    void finished();

public slots:
    void createModel(QString fileName);
    void setup();
    void runOneStep();
    void run(int n_steps);
    void abort();
    ModelRunState state() { return mState; }
    QString stateString(ModelRunState s);

    void processedPackage(std::list<InferenceData*>*package, int packageId);
    void allPackagesBuilt();

private:
    void internalRun();
    void buildInferenceData(Cell *cell);
    void addDataPackage(InferenceData *item);
    void sendInferencePackage();
    void finalizeCycle();


    void setState(ModelRunState new_state, QString msg=QString());
    ModelRunState mState; // current state of the model
    bool mAbort;
    Model *mModel;

    std::list< std::list<InferenceData*>* > mInferenceData;
    int mLivePackages;
    int mPackageId;

    QFutureWatcher<void> packageWatcher;
    QFuture<void> packageFuture;

    // loggers
    std::shared_ptr<spdlog::logger> lg;



};

#endif // MODELSHELL_H
