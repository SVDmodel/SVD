/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#include "testdnn.h"
#include "ui_testdnn.h"

#include <QDebug>
#include <QFileDialog>
#include <QImageReader>

#include "../Predictor/predictortest.h"

PredictorTest model;

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

void TestDNN::on_testGeoTiff_clicked()
{
    qDebug() << QImageReader::supportedImageFormats();
    QImage img("e:/Daten/SVD/projects/gye/gis/dem.tif", "tiff");
    qDebug() << "loaded:" << img.size();

    ui->output->appendPlainText(img.text());
}
