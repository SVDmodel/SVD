#include "modelcontroller.h"
#include "modelshell.h"

ModelController::ModelController(QObject *parent) : QObject(parent)
{
    ModelShell *model_shell = new ModelShell;
    model_shell->moveToThread(&modelThread);

    connect(&modelThread, &QThread::finished, model_shell, &QObject::deleteLater);

    //connect(this, &ModelController::operate, model_shell, &ModelShell::doWork);
    //connect(model_shell, &ModelShell::resultReady, this, &ModelController::handleResults);

    // logging
    connect(model_shell, &ModelShell::log, this, &ModelController::log);
    log("Modelcontroller: before thread.start()");
    modelThread.start();
}

ModelController::~ModelController()
{
    modelThread.quit();
    modelThread.wait();
    log("Destroyed Thread");
}
