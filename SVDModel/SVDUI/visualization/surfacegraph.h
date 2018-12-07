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

#ifndef SURFACEGRAPH_H
#define SURFACEGRAPH_H

#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/QSurface3DSeries>
#include <QtWidgets/QSlider>
#include "topographicseries.h"
//#include "highlightseries.h"

//#include "custominputhandler.h"

#include "grid.h"

// using namespace QtDataVisualization;

class SurfaceGraph : public QWidget
{
    Q_OBJECT
public:
    explicit SurfaceGraph(QWidget *parent=nullptr);
    ~SurfaceGraph();

    void setFilename(QString grid_file_name);
    void setup(Grid<float> &dem, float min_h, float max_h);

    void toggleSurfaceTexture(bool enable);
    void clickCamera();

    QtDataVisualization::Q3DSurface *graph() { return m_graph; }
    TopographicSeries *topoSeries() { return m_topography; }
signals:
    void cameraChanged();
private:
    QtDataVisualization::Q3DSurface *m_graph;

    TopographicSeries *m_topography;
    // HighlightSeries *m_highlight;
    int m_highlightWidth;
    int m_highlightHeight;

    //float m_areaWidth;
    //float m_areaHeight;

    Grid<float> *mDem;

    // CustomInputHandler *m_inputHandler;
};

#endif // SURFACEGRAPH_H
