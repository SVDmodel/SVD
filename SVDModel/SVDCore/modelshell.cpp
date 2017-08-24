#include "modelshell.h"
#include "toymodel.h"
#include "model.h"

#include <QThread>
#include <QCoreApplication>

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
    mState = Invalid;
    mModel = 0;
}

ModelShell::~ModelShell()
{
    destroyModel();
}

void ModelShell::destroyModel()
{
    if (mModel) {
        Model *m = mModel;
        mModel = nullptr;
        delete m;
    }
}

void ModelShell::setup(QString fileName)
{
    try {
        setState(Creating);
        if (!model()) {
            mModel = new Model();
        }
        if (model()->setup( fileName.toStdString() )) {
            // setup successful
            auto lg = spdlog::get("main");
            lg->info("Model successfully set up in Thread {}.", QThread::currentThread()->objectName().toStdString());

            // just wait a little... 3secs
            QThread::msleep(3000);
            setState(ReadyToRun);
        }


    } catch (const std::exception &e) {
        if (spdlog::get("main"))
            spdlog::get("main")->error("An error occured: {}", e.what());
        setState( ErrorDuringSetup, QString(e.what()));
    }
}

void ModelShell::runOneStep()
{
    setState(Running);
    try {
        spdlog::get("main")->info("Run one step.");
        // run the model...
        QThread::msleep( 2000 );

        setState(ReadyToRun);

    } catch (const std::exception &e) {
        spdlog::get("main")->error("An error occured: {}", e.what());
        setState( Error, QString(e.what()));
    }
}

void ModelShell::run(int n_steps)
{
    setState(Running);
    try {
        spdlog::get("main")->info("Run one step.");
        // run the model...
        for (int i=0;i<n_steps;++i) {
            QCoreApplication::processEvents();
            spdlog::get("main")->info("Run step {} of {}.", i+1, n_steps);
            setState(Running, QString("year %1 of %2.").arg(i+1).arg(n_steps));
            if (mAbort) {
                setState(Canceled);
                return;
            }

            QThread::msleep(1000);


        }
        setState(ReadyToRun);


    } catch (const std::exception &e) {
        spdlog::get("main")->error("An error occured: {}", e.what());
        setState( Error, QString(e.what()));
    }

}

void ModelShell::abort()
{
    mAbort = true;
    if (mState==Creating || mState==Running) {
        setState(Stopping, "by user request.");
    } else {
        setState(Invalid, "model destroyed");
        destroyModel();
    }
}

QString ModelShell::stateString(ModelRunState s)
{
    // Invalid=0, Creating, ReadyToRun, Stopping, Running, Paused, Finsihed, Canceled, ErrorDuringSetup, Error };
    switch (s) {
    case Invalid: return QStringLiteral("Invalid");
    case Creating: return QStringLiteral("Creating...");
    case ReadyToRun: return QStringLiteral("Ready.");
    case Stopping: return QStringLiteral("Stopping...");
    case Running: return QStringLiteral("Running...");
    case Finsihed: return QStringLiteral("Finished!");
    case Canceled: return QStringLiteral("Canceled!");
    case ErrorDuringSetup: return QStringLiteral("Error during setup!!");
    case Error: return QStringLiteral("Error!!");
    default: return QStringLiteral("invalid state");
    }

}

void ModelShell::setState(ModelShell::ModelRunState new_state, QString msg)
{
    mState = new_state;
    QString text = stateString(mState);
    if (msg.isEmpty())
        emit stateChanged(text);
    else
        emit stateChanged(text + " - " + msg);
}
