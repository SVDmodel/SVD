#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>

#include "testdnn.h"

#include "modelcontroller.h"
ModelController *mc=0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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
