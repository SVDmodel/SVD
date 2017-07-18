#include "modelshell.h"
#include "toymodel.h"

#include <QThread>
#include <QCoreApplication>

ModelShell::ModelShell(QObject *parent) : QObject(parent)
{
    mAbort = false;
    emit log("Model shell created.");
}

ModelShell::~ModelShell()
{
    emit log("Model shell destroyed");
}

void ModelShell::run()
{
    emit log(QString("Running thread %1").arg(QThread::currentThread()->objectName()));

    toy.run();

    emit log("Finished run");
}

void ModelShell::runTest()
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

void ModelShell::process(int n)
{
    for (int i=0;i<n;++i)
        emit log(QString("iteration %1").arg(i));
}

void ModelShell::abort()
{
    mAbort = true;
    toy.abort();
}

