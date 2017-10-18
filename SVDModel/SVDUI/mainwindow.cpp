#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QTimer>
#include <QTime>
#include <QMessageBox>

#include "testdnn.h"

#include "modelcontroller.h"
#include "model.h"
#include "modelrunstate.h"

#include "integratetest.h"
#include "../Predictor/predtest.h"

#include "spdlog/spdlog.h"
#include "strtools.h"


ToyModelController *mc=0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // start basic logging
    //initiateLogging();

    // start up the model shell
    mMC = std::unique_ptr<ModelController>(new ModelController());
    initiateModelController();


}


MainWindow::~MainWindow()
{

    auto l = spdlog::get("main");
    if (l)
        l->info("Shutdown of the application.");
    spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l)
    {
        l->flush();
    });
    //spdlog::drop_all();
    delete ui;
}

void MainWindow::modelStateChanged(QString s)
{
    // stop the update timer...
    if (!mMC->state()->isModelRunning()) {
        mUpdateModelTimer.stop();
        modelUpdate();
    }

}

void MainWindow::modelUpdate()
{
    ui->lModelState->setText(QString("%1 - %2").arg( QTime::currentTime().toString(Qt::ISODate) ).arg(QString::fromStdString(RunState::instance()->asString())));
    on_pbUpdateStats_clicked();
}


void MainWindow::on_actionTest_DNN_triggered()
{
    // open test DNN form and show model
    TestDNN *testdnn = new TestDNN();
    testdnn->show();

}

void MainWindow::on_pbStart_clicked()
{
    if (!mc) {
        ui->lLog->appendPlainText("Starting ModelController...");
        mc = new ToyModelController(this);
        connect(mc, &ToyModelController::log, ui->lLog, &QPlainTextEdit::appendPlainText);
    }
}

void MainWindow::on_pbStop_clicked()
{
    if (mc) {
        ui->lLog->appendPlainText("Stopping ModelController...");
        delete mc;
        mc=0;
    }
}

void MainWindow::on_run_clicked()
{
    if (mc)
        mc->run();
}

void MainWindow::on_pushButton_clicked()
{
    if (mc)
        mc->abort();
}

void MainWindow::on_pbTest_clicked()
{
    IntegrateTest it;
    it.testGrid();
    qDebug() << "test end";
}

void MainWindow::on_pushButton_2_clicked()
{
    IntegrateTest it;
    it.testSettings();
}


class my_threaded_sink : public spdlog::sinks::base_sink < std::mutex >
{
public:
    my_threaded_sink(QPlainTextEdit *output) { mOut = output; }
protected:
    void _sink_it(const spdlog::details::log_msg& msg) override
    {
        //Your code here
        char *buf = const_cast<char *>(msg.formatted.c_str());
        // search for \r
        for (char *p=buf; *p!=0; p++)
            if (*p == '\r') {
                *p = '\0';
                break;
            }

        // QMetaObject::invokeMethod(mOut, "appendPlainText", Qt::QueuedConnection, Q_ARG(QString, QString(buf)));
        //mOut->appendPlainText( QString(buf));
    }
    void _flush() override
    {
        // do nothing
        //mOut->appendPlainText(".... flushing .....");
    }

private:
    QPlainTextEdit *mOut;
};

void MainWindow::on_pushButton_3_clicked()
{




    IntegrateTest it;
    //auto console = spdlog::basic_logger_mt("console", "log.txt");

    it.testLogging();
}


void MainWindow::initiateLogging()
{
    if (spdlog::get("main"))
        return;
    // asynchronous logging, 2 seconds auto-flush
    spdlog::set_async_mode(8192, spdlog::async_overflow_policy::block_retry,
                           nullptr,
                           std::chrono::seconds(2));

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>("log.txt"));
    sinks.push_back(std::make_shared<my_threaded_sink>(ui->lLog));
    auto combined_logger = spdlog::create("main", sinks.begin(), sinks.end());
    combined_logger->set_level(spdlog::level::debug);
    combined_logger->flush_on(spdlog::level::err);

    combined_logger=spdlog::create("dnn", sinks.begin(), sinks.end());
    combined_logger->set_level(spdlog::level::debug);
    combined_logger->flush_on(spdlog::level::err);

    //auto combined_logger = std::make_shared<spdlog::logger>("console", begin(sinks), end(sinks));

    //register it if you need to access it globally
    //spdlog::register_logger(combined_logger);

    combined_logger->info("Started logging");
}

void MainWindow::initiateModelController()
{
    // bookkeeping, signal - slot connections
    connect(mMC.get(), &ModelController::stateChanged, [this](QString s) {ui->statusBar->showMessage(s);});
    connect(mMC.get(), &ModelController::stateChanged, this, &MainWindow::modelStateChanged);
    connect(mMC.get(), &ModelController::finishedYear, ui->progressBar, &QProgressBar::setValue);
    connect(mMC.get(), &ModelController::finished, [this]() { ui->progressBar->setValue(ui->progressBar->maximum());});

    connect(&mUpdateModelTimer, &QTimer::timeout, this, &MainWindow::modelUpdate);

    //connect(mMC.get(), &ModelController::stateChanged, ui->statusBar, &QStatusBar::showMessage);
    //QObject::connect(mMC.get(), SIGNAL(modelState(QString)), ui->statusBar, SLOT(showMessage(QString,int)));
}

void MainWindow::on_pushButton_4_clicked()
{
    IntegrateTest it;
    it.testRandom();
}

void MainWindow::on_pushButton_5_clicked()
{
    initiateLogging();
    PredTest it;
    it.testTensor();
}

void MainWindow::on_pbLoad_clicked()
{
    // create & load model
    if (mMC && mMC->model()) {
        if (QMessageBox::question(this, "Confirm reload", "The model is already created. Create a new model?")==QMessageBox::No)
            return;
    }

    mMC.reset();
    mMC.reset(new ModelController()); // this frees the current model
    initiateModelController();

    mMC->setup(ui->lConfigFile->text());
    mUpdateModelTimer.start(100);
}

void MainWindow::on_pbDeleteModel_clicked()
{
    mMC->shutdown();
}

void MainWindow::on_pbRunModel_clicked()
{
    ui->progressBar->reset();
    ui->progressBar->setMaximum( ui->sYears->value() );
    mMC->run( ui->sYears->value() );
    mUpdateModelTimer.start(100);
}

void MainWindow::on_pbRun_clicked()
{
    std::string s = mMC->shell()->run_test_op(ui->cbOption->currentText().toStdString());
    writeFile(ui->lParam->text().toStdString(), s);

}



void MainWindow::on_pbUpdateStats_clicked()
{
    auto stats = mMC->systemStatus();
    ui->listStatus->clear();
    ui->listStatus->setHorizontalHeaderLabels({"Statistic", "Value"});
    ui->listStatus->setRowCount(stats.size());
    ui->listStatus->setColumnCount(2);
    int r=0;
    for (std::pair<std::string, std::string> s : stats) {
        ui->listStatus->setItem(r,0, new QTableWidgetItem(QString::fromStdString(s.first)));
        ui->listStatus->setItem(r,1, new QTableWidgetItem(QString::fromStdString(s.second)));
        ++r;
    }
}
