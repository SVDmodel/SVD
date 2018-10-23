#ifndef CAMERACONTROL_H
#define CAMERACONTROL_H

#include <QDialog>
#include "surfacegraph.h"

namespace Ui {
class CameraControl;
}

class CameraControl : public QDialog
{
    Q_OBJECT

public:
    explicit CameraControl(QWidget *parent = nullptr);
    ~CameraControl();

    void setSurfaceGraph(SurfaceGraph *s) { mSurface=s; }

public slots:
    void cameraChanged();
private slots:
    void on_targetX_actionTriggered(int action);
    void on_targetY_actionTriggered(int action);
    void on_targetZ_actionTriggered(int action);

    void on_zoomFactor_actionTriggered(int action);

    void on_zFactor_actionTriggered(int action);

private:
    void updateCamera();
    Ui::CameraControl *ui;
    SurfaceGraph *mSurface;
};


#endif // CAMERACONTROL_H
