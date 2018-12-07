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

#include "cameracontrol.h"
#include "ui_cameracontrol.h"

CameraControl::CameraControl(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CameraControl)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose); // delete automatically on close
}

CameraControl::~CameraControl()
{
    delete ui;
}

void CameraControl::cameraChanged()
{
    QtDataVisualization::Q3DCamera *camera = mSurface->graph()->scene()->activeCamera();
    if (!camera)
        return;
    ui->targetX->setValue(static_cast<int>(camera->target().x()*100.));
    ui->targetY->setValue(static_cast<int>(camera->target().y()*100.));
    ui->targetZ->setValue(static_cast<int>(camera->target().z()*100.));
    ui->zoomFactor->setValue(static_cast<int>(camera->zoomLevel()));
    ui->rotationX->setText(QString::number(camera->xRotation()));
    ui->rotationY->setText(QString::number(camera->yRotation()));

    QString cam_string = QString("%1,%2,%3,%4,%5,%6").arg(camera->target().x()).arg(camera->target().y()).arg(camera->target().z())
            .arg(camera->zoomLevel()).arg(camera->xRotation()).arg(camera->yRotation());
    ui->lCamPos->setText(cam_string);
}

void CameraControl::on_targetX_actionTriggered(int action)
{
    Q_UNUSED(action)
    updateCamera();
}

void CameraControl::on_targetY_actionTriggered(int action)
{
    Q_UNUSED(action)
    updateCamera();
}

void CameraControl::on_targetZ_actionTriggered(int action)
{
    Q_UNUSED(action)
    updateCamera();
}


void CameraControl::updateCamera()
{

    QtDataVisualization::Q3DCamera *camera = mSurface->graph()->scene()->activeCamera();
    QVector3D new_target(
                ui->targetX->value()/100.f,
                ui->targetY->value()/100.f,
                ui->targetZ->value()/100.f
                );
    camera->setMinZoomLevel(0.f); camera->setMaxZoomLevel(10000.f);
    camera->setTarget(new_target);
    camera->setZoomLevel( ui->zoomFactor->value() );
    qDebug() << "Camera: target: " << camera->target() << ", zoom:" << camera->zoomLevel() << "aspect:" << mSurface->graph()->aspectRatio();
}

void CameraControl::on_zoomFactor_actionTriggered(int action)
{
    Q_UNUSED(action)
    updateCamera();
}

void CameraControl::on_zFactor_actionTriggered(int action)
{
    Q_UNUSED(action)
    mSurface->graph()->setAspectRatio( ui->zFactor->value() / 100.);
    updateCamera();
}

void CameraControl::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void CameraControl::on_pbSetFromString_clicked()
{
    QStringList elem = ui->lCamPos->text().split(",");
    if (elem.size()!=6)
        return;
    QtDataVisualization::Q3DCamera *camera = mSurface->graph()->scene()->activeCamera();

    QVector3D new_pos = QVector3D(elem[0].toDouble(),
            elem[1].toDouble(),
            elem[2].toDouble());
    camera->setTarget(new_pos);
    camera->setZoomLevel(elem[3].toDouble());
    camera->setXRotation(elem[4].toDouble());
    camera->setYRotation(elem[5].toDouble());
}
