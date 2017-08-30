#ifndef MODELCONTROLLER_H
#define MODELCONTROLLER_H

#include <QObject>
#include <QThread>
#include "modelshell.h"


class ToyModelShell; // forward

class ToyModelController : public QObject
{
    Q_OBJECT
    QThread *modelThread;
    QThread *dnnThread;
public:
    explicit ToyModelController(QObject *parent = nullptr);
    ~ToyModelController();

signals:
    void log(const QString &s);

public slots:
    void run();
    void abort();
    void finishedRun();
private:
    ToyModelShell *mModel;

};

class ModelInterface; // forward


class ModelController : public QObject
{
    Q_OBJECT
    QThread *modelThread;
    QThread *dnnThread;
public:
    explicit ModelController(QObject *parent = nullptr);
    ~ModelController();
    ModelShell *shell() const { return mModelShell; }
    const Model *model() const { return mModelShell->model(); }
    ModelInterface *dnnInterface() const { return mModelInterface; }

signals:
    void log(const QString &s);
    void stateChanged(QString s);

public slots:
    void setup(QString fileName);
    void shutdown();

    void run(int n_years);
    //void abort();
    void finishedRun();
private:
    ModelShell *mModelShell; // lives in main thread
    ModelInterface *mModelInterface; // lives in DNN thread

};

#endif // MODELCONTROLLER_H
