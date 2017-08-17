#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>

#include "testdnn.h"

#include "modelcontroller.h"

#include "integratetest.h"

#include "spdlog/spdlog.h"


ModelController *mc=0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // start basic logging
    initiateLogging();
}


MainWindow::~MainWindow()
{
    delete ui;
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
        mc = new ModelController(this);
        connect(mc, &ModelController::log, ui->lLog, &QPlainTextEdit::appendPlainText);
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

        QMetaObject::invokeMethod(mOut, "appendPlainText", Qt::QueuedConnection, Q_ARG(QString, QString(buf)));
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
    // asynchronous logging, 2 seconds auto-flush
    spdlog::set_async_mode(8192, spdlog::async_overflow_policy::block_retry,
                           nullptr,
                           std::chrono::seconds(2));

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>("log.txt"));
    sinks.push_back(std::make_shared<my_threaded_sink>(ui->lLog));
    auto combined_logger = spdlog::create("main", sinks.begin(), sinks.end());
    combined_logger->set_level(spdlog::level::debug);
    combined_logger=spdlog::create("dnn", sinks.begin(), sinks.end());
    combined_logger->set_level(spdlog::level::debug);
    //auto combined_logger = std::make_shared<spdlog::logger>("console", begin(sinks), end(sinks));

    //register it if you need to access it globally
    //spdlog::register_logger(combined_logger);

    combined_logger->info("Started logging");
}
