#ifndef MODELSHELL_H
#define MODELSHELL_H

#include <QObject>
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

signals:
    void stateChanged(QString s);
    void processedStep(int n);
    void log(QString s);
public slots:
    void setup(QString fileName);
    void runOneStep();
    void run(int n_steps);
    void abort();
    ModelRunState state() { return mState; }
    QString stateString(ModelRunState s);
private:
    void setState(ModelRunState new_state, QString msg=QString());
    ModelRunState mState; // current state of the model
    bool mAbort;
    Model *mModel;

};

#endif // MODELSHELL_H
