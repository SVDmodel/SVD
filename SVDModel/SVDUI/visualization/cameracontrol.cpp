#include "cameracontrol.h"
#include "ui_cameracontrol.h"

CameraControl::CameraControl(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CameraControl)
{
    ui->setupUi(this);
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
