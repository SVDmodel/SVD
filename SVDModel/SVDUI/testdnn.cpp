#include "testdnn.h"
#include "ui_testdnn.h"

#include <QDebug>
#include <QFileDialog>

#include "../Predictor/predictor.h"

Predictor model;

TestDNN::TestDNN(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TestDNN)
{
    ui->setupUi(this);
}

TestDNN::~TestDNN()
{
    delete ui;
}

void TestDNN::on_runInception_clicked()
{
    //QStringList argv = { "inception" };
    //char *argv[] = { "inception" };

    //int result = inception(1, argv);

}

void TestDNN::on_setupModel_clicked()
{
    if (model.setup(ui->modelPath->text()))
        qDebug() << "setup successful";
}

void TestDNN::on_selectFile_clicked()
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

void TestDNN::on_doPredict_clicked()
{

    //QString text = model.insight();
    //ui->output->appendPlainText(text);
    QString text = model.runModel();
    ui->output->appendPlainText(text);
/*    QString fileName = ui->imagePath->text();
    for (int i=0;i<1000;++i) {
        QString result = model.classifyImage(fileName);
        ui->output->appendPlainText(result);
    }
    ui->output->appendPlainText("*** executed 1000 times *** !!"); */
}
