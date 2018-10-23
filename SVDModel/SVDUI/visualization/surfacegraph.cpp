/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Data Visualization module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "surfacegraph.h"
#include "topographicseries.h"

#include <QtDataVisualization/QValue3DAxis>
#include <QtDataVisualization/Q3DTheme>
#include <QtDataVisualization/Q3DSurface>

#include <QPainter>
#include <QMessageBox>
#include <QScreen>
#include <QLayout>

//using namespace QtDataVisualization;

//const float areaWidth = 8000.0f;
//const float areaHeight = 8000.0f;
const float aspectRatio = 0.1389f;
//const float minRange = areaWidth * 0.49f;

SurfaceGraph::SurfaceGraph(QWidget *parent) : QWidget(parent)
{

    QtDataVisualization::Q3DSurface *graph = new QtDataVisualization::Q3DSurface();
    m_graph = graph;
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

}

SurfaceGraph::~SurfaceGraph()
{
    delete m_graph;
}

void SurfaceGraph::setFilename(QString grid_file_name)
{
    m_Grid.loadGridFromFile(grid_file_name.toStdString());
    setup();

}

void SurfaceGraph::setup()
{
    // load grid
    //m_areaWidth = m_Grid.metricSizeX();
    //m_areaHeight = m_Grid.metricSizeY();
    float max_h = m_Grid.max();
    float min_h = m_Grid.min();

    //const float aspectRatio = 0.1389f;
    //const float minRange = m_areaWidth * 0.49f;


    m_graph->setAxisX(new QtDataVisualization::QValue3DAxis);
    m_graph->setAxisY(new QtDataVisualization::QValue3DAxis);
    m_graph->setAxisZ(new QtDataVisualization::QValue3DAxis);
    m_graph->axisX()->setLabelFormat("%i");
    m_graph->axisZ()->setLabelFormat("%i");
    m_graph->axisX()->setRange(0.0f, m_Grid.metricSizeX());
    m_graph->axisY()->setRange(min_h, max_h*6);
    m_graph->axisZ()->setRange(0.0f, m_Grid.metricSizeY());
    m_graph->axisX()->setLabelAutoRotation(30);
    m_graph->axisY()->setLabelAutoRotation(90);
    m_graph->axisZ()->setLabelAutoRotation(30);

    //m_graph->axisY()->setSegmentCount(1);

    m_graph->axisY()->setTitleVisible(false);

    m_graph->activeTheme()->setType(QtDataVisualization::Q3DTheme::ThemePrimaryColors);

    QFont font = m_graph->activeTheme()->font();
    font.setPointSize(12);
    m_graph->activeTheme()->setFont(font);

    m_topography = new TopographicSeries();
    // m_topography->setTopographyFile(":/maps/topography", m_areaWidth, m_areaHeight);

    m_topography->setGrid(m_Grid, min_h);

    m_topography->setItemLabelFormat(QStringLiteral("@yLabel m"));

/*    m_highlight = new HighlightSeries();
    m_highlight->setTopographicSeries(m_topography);
    m_highlight->setMinHeight(600.0f); // minRange * aspectRatio
    m_highlight->handleGradientChange(max_h);
//! [1]
    QObject::connect(m_graph->axisY(), &QValue3DAxis::maxChanged,
                     m_highlight, &HighlightSeries::handleGradientChange);
//! [1] */

    m_graph->addSeries(m_topography);
/*    m_graph->addSeries(m_highlight);

    m_inputHandler = new CustomInputHandler(m_graph);
    m_inputHandler->setHighlightSeries(m_highlight);
    m_inputHandler->setAxes(m_graph->axisX(), m_graph->axisY(), m_graph->axisZ());
    m_inputHandler->setLimits(0.0f, m_Grid.metricSizeX(), max_h); // 0.0f, m_areaWidth, minRange
    m_inputHandler->setAspectRatio(aspectRatio);

    m_graph->setActiveInputHandler(m_inputHandler); */


    // custom themeing


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


}

//! [0]
void SurfaceGraph::toggleSurfaceTexture(bool enable)
{
    QLinearGradient gr(0,0,1000,1);

    gr.setColorAt(0.0, Qt::black);
    gr.setColorAt(0.33, Qt::blue);
    gr.setColorAt(0.67, Qt::red);
    gr.setColorAt(1.0, Qt::yellow);
    //gr.setCoordinateMode(QGradient::StretchToDeviceMode);
    QBrush brush(gr);
    QImage color_map(1000, 10, QImage::Format_ARGB32_Premultiplied);

    QPainter painter(&color_map);
    painter.setBrush(brush);
    painter.drawRect(color_map.rect());
    //painter.fillRect(color_map.rect(), brush);
    painter.end();
    color_map.save("test.jpg");


    //QImage img = m_topography->texture();
    QImage img = m_topography->texture();
    bool new_img = false;
    if (img.isNull()) {
        img = QImage(m_Grid.sizeX(), m_Grid.sizeY(), QImage::Format_ARGB32_Premultiplied);
        new_img = true;
    }
    for (int x=0;x<img.width();++x)
        for (int y=0;y<img.height(); ++y) {
            if (new_img) {
                float value = y/float(img.height())*y/float(img.height());
                QRgb col = color_map.pixel( value * color_map.width(),1);
                //QRgb col = qRgba( x % 256, y % 256, 127, int(120+x/10));
                if (m_Grid(x,y)>1189.f)
                    img.setPixel(x,img.height()-y-1,col);
                else
                    img.setPixel(x,img.height()-y-1,qRgba(255,255,255,127));
            } else {
                QColor col(img.pixel(x,y));
                col.setRed( (col.red()+1) % 255);
                img.setPixel(x,y, col.rgba() );

            }
        }

    m_topography->setTexture(img);
    return;

    if (enable)
        m_topography->setTextureFile(":/maps/maptexture");
    else
        m_topography->setTextureFile("");
}

void SurfaceGraph::clickCamera()
{
    qDebug() << "click";
    int preset = int(m_graph->scene()->activeCamera()->cameraPreset());
    qDebug() << preset;
    // m_graph->scene()->activeCamera()->setCameraPreset(QtDataVisualization::Q3DCamera::CameraPreset( preset + 1) );
    QVector3D target = m_graph->scene()->activeCamera()->target();
    target.setX(target.y() - 0.1);
    m_graph->scene()->activeCamera()->setTarget(target);
    m_graph->scene()->activeCamera()->setMaxZoomLevel(2000);
    m_graph->scene()->activeCamera()->setZoomLevel( m_graph->scene()->activeCamera()->zoomLevel() + 100 );
}
//! [0]
