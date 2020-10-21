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
#include "landscapevisualization.h"

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
    ui->zFactor->setValue(static_cast<int>(mSurface->graph()->aspectRatio()*100));
    ui->yMaxRange->setValue(static_cast<int>(mSurface->graph()->axisY()->max()/1000.));

    QStringList res;
    res << QString("xRotation=%1").arg(camera->xRotation())
        << QString("yRotation=%1").arg(camera->yRotation())
        << QString("zoomLevel=%1").arg(camera->zoomLevel())
        << QString("targetX=%1").arg(camera->target().x())
        << QString("targetY=%1").arg(camera->target().y())
        << QString("targetZ=%1").arg(camera->target().z())
        << QString("aspectRatio=%1").arg(mSurface->graph()->aspectRatio())
        << QString("maxYAxis=%1").arg(mSurface->graph()->axisY()->max());
    ui->cameraText->setPlainText(res.join("\n"));

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


void CameraControl::on_yMaxRange_actionTriggered(int action)
{
    Q_UNUSED(action)
    float max_range = ui->yMaxRange->value()*1000.f;
    mSurface->graph()->axisY()->setRange(mSurface->graph()->axisY()->min(), std::max(mSurface->graph()->axisY()->min(), max_range));
    qDebug() << "max-range:" << max_range/1000. << "km";
}

void CameraControl::on_mbSetBGColor_clicked()
{
    QString colstr = ui->lBGColor->text();
    QColor col(colstr);
    if (col.isValid())
        mLandscapeVis->setFillColor(col);
    else {
        qDebug()<< "invalid color:" << colstr;
    }
}
