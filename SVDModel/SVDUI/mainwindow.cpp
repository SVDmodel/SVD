#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>

#include "../Predictor/predictor.h"

Predictor model;

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

void MainWindow::on_runInception_clicked()
{
    //QStringList argv = { "inception" };
    //char *argv[] = { "inception" };

    //int result = inception(1, argv);

}

void MainWindow::on_setupModel_clicked()
{
    if (model.setup(ui->modelPath->text()))
        qDebug() << "setup successful";
}

void MainWindow::on_selectFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
          tr("Open Image"), "/home/werner", tr("Image Files (*.png *.jpg *.bmp)"));
    if (!fileName.isEmpty()) {
        ui->imagePath->setText(fileName);
        QImage img;
        img.load(fileName);
        ui->image->setPixmap(QPixmap::fromImage(img));
        QString result = model.classifyImage(fileName);
        ui->output->appendPlainText(result);
    }
}

void MainWindow::on_doPredict_clicked()
{
    QString fileName = ui->imagePath->text();
    for (int i=0;i<1000;++i) {
        QString result = model.classifyImage(fileName);
        ui->output->appendPlainText(result);
    }
    ui->output->appendPlainText("*** executed 1000 times *** ");
}
