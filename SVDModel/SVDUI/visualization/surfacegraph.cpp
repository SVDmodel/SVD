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


#include "surfacegraph.h"
#include "topographicseries.h"
#include "custom3dinputhandler.h"

#include <QtDataVisualization/QValue3DAxis>
#include <QtDataVisualization/Q3DTheme>
#include <QtDataVisualization/Q3DSurface>

#include <QPainter>
#include <QMessageBox>
#include <QScreen>
#include <QLayout>


// dummy axis formatter
class DummyAxisFormatter:  public QtDataVisualization::QValue3DAxisFormatter {
public:
    virtual QValue3DAxisFormatter *createNewInstance() const { return new DummyAxisFormatter();}
    virtual void recalculate() {}
    virtual QStringList &labelStrings() const { return empty_list; }
    virtual QVector<float> &labelPositions() const { return empty_vec; }
private:
    mutable QStringList empty_list;
    mutable QVector<float> empty_vec;
};


SurfaceGraph::SurfaceGraph(QWidget *parent) : QWidget(parent)
{

    QtDataVisualization::Q3DSurface *graph = new QtDataVisualization::Q3DSurface();
    m_graph = graph;
    m_topography = nullptr;
    //QWidget::createWindowContainer();

    QWidget *container = QWidget::createWindowContainer(graph);
    QSize screenSize = graph->screen()->size();

    container->setMinimumSize(QSize(400, 300));
    container->setMaximumSize(screenSize);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    container->setFocusPolicy(Qt::StrongFocus);
    container->setParent(this);

    QHBoxLayout *hLayout = new QHBoxLayout(this);
    QVBoxLayout *vLayout = new QVBoxLayout();
    hLayout->addWidget(container, 1);
    hLayout->addLayout(vLayout);
    vLayout->setAlignment(Qt::AlignTop);
    hLayout->setMargin(0);
    vLayout->setMargin(0);

    if (!graph->hasContext()) {
        QMessageBox msgBox;
        msgBox.setText("Couldn't initialize the OpenGL context.");
        msgBox.exec();
    }

    m_graph->setAxisX(new QtDataVisualization::QValue3DAxis);
    m_graph->setAxisY(new QtDataVisualization::QValue3DAxis);
    m_graph->setAxisZ(new QtDataVisualization::QValue3DAxis);

    m_graph->axisX()->setLabelAutoRotation(30);
    m_graph->axisY()->setLabelAutoRotation(90);
    m_graph->axisZ()->setLabelAutoRotation(30);

    m_graph->axisY()->setTitleVisible(false);


    //DummyAxisFormatter *af = new DummyAxisFormatter();
    m_graph->axisX()->setFormatter(new DummyAxisFormatter);
    m_graph->axisY()->setFormatter(new DummyAxisFormatter);
    m_graph->axisZ()->setFormatter(new DummyAxisFormatter);


    m_graph->activeTheme()->setType(QtDataVisualization::Q3DTheme::ThemePrimaryColors);

    QFont font = m_graph->activeTheme()->font();
    font.setPointSize(12);
    m_graph->activeTheme()->setFont(font);

    QtDataVisualization::Q3DTheme *theme = new QtDataVisualization::Q3DTheme(QtDataVisualization::Q3DTheme::ThemeDigia);
    // theme->setAmbientLightStrength(0.3f);
    //theme->setBackgroundColor(Qt::white);
    theme->setBackgroundEnabled(false);
    //theme->setBaseColor(QColor(QRgb(0x209fdf)));
    //theme->setColorStyle(Q3DTheme::ColorStyleUniform);
    //theme->setFont(QFont(QStringLiteral("Impact"), 35));
    theme->setGridEnabled(false);
    theme->setLabelTextColor(Qt::white);
    theme->setLabelBorderEnabled(false);
    theme->setLabelBackgroundEnabled(false);

//    theme->setGridLineColor(QColor(QRgb(0x99ca53)));
//    theme->setHighlightLightStrength(7.0f);
//    theme->setLabelBackgroundColor(QColor(0xf6, 0xa6, 0x25, 0xa0));
//    theme->setLabelBackgroundEnabled(true);
//    theme->setLabelBorderEnabled(true);
//    theme->setLabelTextColor(QColor(QRgb(0x404044)));
//    theme->setLightColor(Qt::white);
//    theme->setLightStrength(6.0f);
//    theme->setMultiHighlightColor(QColor(QRgb(0x6d5fd5)));
//    theme->setSingleHighlightColor(QColor(QRgb(0xf6a625)));
//    theme->setWindowColor(QColor(QRgb(0xffffff)));

    m_graph->setActiveTheme(theme);


    QObject::connect(m_graph->scene()->activeCamera(), &QtDataVisualization::Q3DCamera::targetChanged, this, &SurfaceGraph::cameraChanged);
    QObject::connect(m_graph->scene()->activeCamera(), &QtDataVisualization::Q3DCamera::zoomLevelChanged, this, &SurfaceGraph::cameraChanged);
    QObject::connect(m_graph->scene()->activeCamera(), &QtDataVisualization::Q3DCamera::xRotationChanged, this, &SurfaceGraph::cameraChanged);
    QObject::connect(m_graph->scene()->activeCamera(), &QtDataVisualization::Q3DCamera::yRotationChanged, this, &SurfaceGraph::cameraChanged);
    QObject::connect(m_graph, &QtDataVisualization::Q3DSurface::queriedGraphPositionChanged, this, &SurfaceGraph::queryPositionChanged);

    m_graph->setActiveInputHandler(new Custom3dInputHandler());
}

SurfaceGraph::~SurfaceGraph()
{
    delete m_graph;
    qDeleteAll(mDefaultViews);
}



void SurfaceGraph::setup(Grid<float> &dem, float min_h, float max_h)
{

    m_graph->axisX()->setLabelFormat("%i");
    m_graph->axisZ()->setLabelFormat("%i");
    m_graph->axisX()->setRange(0.0f, static_cast<float>(dem.metricSizeX()));
    m_graph->axisY()->setRange(min_h, max_h*40);
    m_graph->axisZ()->setRange(0.0f, static_cast<float>(dem.metricSizeY()));


    m_topography = new TopographicSeries();

    m_topography->setGrid(dem, min_h);

    m_topography->setItemLabelFormat(QStringLiteral("@yLabel m"));


    m_graph->addSeries(m_topography);

    mDefaultViews.clear();
    for (int i=0;i<4;++i) {
        mDefaultViews.push_back(new Q3DCamera());
        mDefaultViews[i]->copyValuesFrom(*m_graph->scene()->activeCamera());
    }

}


void SurfaceGraph::clickCamera()
{
    int preset = int(m_graph->scene()->activeCamera()->cameraPreset());
    qDebug() << preset;
    // m_graph->scene()->activeCamera()->setCameraPreset(QtDataVisualization::Q3DCamera::CameraPreset( preset + 1) );
    QVector3D target = m_graph->scene()->activeCamera()->target();
    target.setX(target.y() - 0.1f);
    m_graph->scene()->activeCamera()->setTarget(target);
    m_graph->scene()->activeCamera()->setMaxZoomLevel(2000);
    m_graph->scene()->activeCamera()->setZoomLevel( m_graph->scene()->activeCamera()->zoomLevel() + 100 );

}

void SurfaceGraph::queryPositionChanged(const QVector3D &pos)
{
    QVector3D world_pos = m_topography->getCoordsFromRelative(pos);
    //spdlog::get("main")->info("Grid: x: {}, y: {}, z: {} World: x: {}, y: {}, z: {} ", pos.x(), pos.y(), pos.z(), world_pos.x(), world_pos.y(), world_pos.z());
    emit pointSelected(world_pos);
}

void SurfaceGraph::resetCameraPosition(int cameraPreset)
{
   if (cameraPreset>=mDefaultViews.length())
       return;

    m_graph->scene()->activeCamera()->copyValuesFrom(*mDefaultViews[cameraPreset]);
   // force a repaint of the scene
   float rot = m_graph->scene()->activeCamera()->xRotation();
   m_graph->scene()->activeCamera()->setXRotation(rot + 1.f);
   m_graph->scene()->activeCamera()->setXRotation(rot);

}

void SurfaceGraph::saveCameraPosition(int cameraPreset)
{
    if (cameraPreset>0 && cameraPreset<mDefaultViews.size()) {
        mDefaultViews[cameraPreset]->copyValuesFrom(*m_graph->scene()->activeCamera());
    }

}
