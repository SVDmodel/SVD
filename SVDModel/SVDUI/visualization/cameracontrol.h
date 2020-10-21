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
#ifndef CAMERACONTROL_H
#define CAMERACONTROL_H

#include <QDialog>
#include "surfacegraph.h"

class LandscapeVisualization;
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
    void setLandscapeVisualization(LandscapeVisualization *lv) { mLandscapeVis = lv; }

public slots:
    void cameraChanged();
private slots:
    void on_targetX_actionTriggered(int action);
    void on_targetY_actionTriggered(int action);
    void on_targetZ_actionTriggered(int action);

    void on_zoomFactor_actionTriggered(int action);

    void on_zFactor_actionTriggered(int action);

    void on_yMaxRange_actionTriggered(int action);

    void on_mbSetBGColor_clicked();

protected:
     void closeEvent(QCloseEvent *event);

private:
    void updateCamera();
    Ui::CameraControl *ui;
    SurfaceGraph *mSurface;
    LandscapeVisualization *mLandscapeVis;
};


#endif // CAMERACONTROL_H
