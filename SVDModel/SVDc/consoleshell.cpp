#include "consoleshell.h"

#include <QCoreApplication>
#include <QFile>
#include <QDebug>

#include "settings.h"


ConsoleShell::ConsoleShell(QObject *parent) : QObject(parent)
{

}

void ConsoleShell::run()
{
    QString config_file_name = QCoreApplication::arguments().at(1);
    // get the number of years to run...
    bool ok;
    mYears = QCoreApplication::arguments().at(2).toInt(&ok);
    if (mYears<0 || !ok) {
        qDebug() << QCoreApplication::arguments().at(2) << "is an invalid number of years to run!";
        QCoreApplication::quit();
        return;
    }

    if (!QFile::exists(config_file_name)) {
        qDebug() << "configuration file does not exist: " << config_file_name;
        QCoreApplication::quit();
        return;
    }

    mStopwatch = QTime::currentTime();

    mController = new ModelController();
    mCreated = false;
    mRunning = false;


    //connect(mController, &ModelController::stateChanged, [this](QString s) {ui->statusBar->showMessage(s);});
    //connect(mController, &ModelController::stateChanged, this, &MainWindow::modelStateChanged);

    connect(mController, &ModelController::finishedYear, this, &ConsoleShell::finishedYear);
    connect(mController, &ModelController::finished, this, &ConsoleShell::finished);

    //connect(mController, &ModelController::finishedYear, mLandscapeVis, &LandscapeVisualization::update);
    //connect(mController, &ModelController::finished, mLandscapeVis, &LandscapeVisualization::update);

    connect(&mUpdateModelTimer, &QTimer::timeout, this, &ConsoleShell::modelUpdate);


    Settings local_settings;


    try {

        if (!local_settings.loadFromFile(config_file_name.toStdString())) {
            throw std::logic_error("Error in loading configuration file: " + config_file_name.toStdString());
        }

        mParams.clear();
        if (QCoreApplication::arguments().count()>3) {
            qWarning() << "set command line values:";
            for (int i=3;i<QCoreApplication::arguments().count();++i) {
                QString line = QCoreApplication::arguments().at(i);
                line = line.remove(QChar('"')); // drop quotes
                mParams.append(line);
                //qDebug() << qPrintable(line);
                std::string key = line.left(line.indexOf('=')).toStdString();
                std::string value = line.mid(line.indexOf('=')+1).toStdString();

                local_settings.setValue(key, value);

            }
        }
        // set up the model with the modified settings
        mController->setup(  config_file_name,  &local_settings );

        int waitcount = 0;
        printf("Current model state: %s (dnn: %s, model: %s)\n",
               RunState::instance()->state().stateString().c_str(),
               RunState::instance()->dnnState().stateString().c_str(),
               RunState::instance()->modelState().stateString().c_str());
        while (RunState::instance()->state().state() == ModelRunState::Creating && waitcount<100) {
            printf("Waiting # %d Current model state: %s (dnn: %s, model: %s)\n", ++waitcount,
                   RunState::instance()->state().stateString().c_str(),
                   RunState::instance()->dnnState().stateString().c_str(),
                   RunState::instance()->modelState().stateString().c_str());
            QThread::msleep(50);
            QCoreApplication::processEvents();
        }
        printf("Finished setup -  state: %s (dnn: %s, model: %s)\n",
               RunState::instance()->state().stateString().c_str(),
               RunState::instance()->dnnState().stateString().c_str(),
               RunState::instance()->modelState().stateString().c_str());


        mUpdateModelTimer.start(100); // 100

    }  catch (const std::exception &e) {
        qWarning() << "*** An error occured ***";
        qWarning() << e.what();
        QCoreApplication::quit();

    }


}

void ConsoleShell::runModel()
{
    try {

        mRunning=true;

        qWarning() << "**************************************************";
        qWarning() << "*** running model for" << mYears << "years";
        qWarning() << "**************************************************";

        mController->run( mYears );
        mUpdateModelTimer.start(100);

        // runJavascript("onFinish");


    }  catch (const std::exception &e) {
    qWarning() << "*** An error occured ***";
    qWarning() << e.what();
}
}

void ConsoleShell::modelUpdate()
{
    //int stime = ui->lModelState->property("starttime").toTime().elapsed();
    //QTime().addMSecs(stime).toString(Qt::ISODateWithMs)
    //ui->lModelState->setText(QString("%1 - %2").arg( QTime(0,0).addMSecs(stime).toString(Qt::ISODate) ).arg(QString::fromStdString(RunState::instance()->asString())));

    int stime = mStopwatch.elapsed();

    printf("\r %s: %s                        ", QTime(0,0).addMSecs(stime).toString(Qt::ISODate).toStdString().c_str(), RunState::instance()->asString().c_str());




    if (mController->state()->isModelCreated() && mCreated==false) {
        printf("\n model successfully created. Starting the simulation.\n");
        mCreated=true;
        runModel();
        return;
    }

    if (mController->state()->isModelFinished() || mController->state()->isModelPaused()) {
        mUpdateModelTimer.stop();
    }
    if (mController->state()->isModelRunning())
        mUpdateModelTimer.start(100);

    if (mController->state()->isError()) {
        printf("An error occured. Stopping.");
        QCoreApplication::quit();
    }
}

void ConsoleShell::finishedYear(int year)
{
    printf("\nyear %d finished.\n", year);
}

void ConsoleShell::finished()
{
    printf("\nTotal time: %s\n", QTime(0,0).addMSecs(mStopwatch.elapsed()).toString(Qt::ISODate).toStdString().c_str());
    printf("********************************\n");
    printf("*****  Simulation finished *****\n");
    printf("********************************\n");
    QCoreApplication::quit();
}
