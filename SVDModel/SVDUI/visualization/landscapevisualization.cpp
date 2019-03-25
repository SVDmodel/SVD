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
#include "landscapevisualization.h"

#include "surfacegraph.h"
#include "topographicseries.h"

#include "modelcontroller.h"
#include "model.h"
#include "expressionwrapper.h"
#include "expression.h"
#include "grid.h"
#include "settings.h"
#include "tools.h"

#include <QPainter>

LandscapeVisualization::LandscapeVisualization(QObject *parent): QObject(parent)
{
    mGraph = nullptr;
    mLegend = nullptr;
    mRenderCount = 0;
    mIsValid = false;
    mIsRendering = false;
    mCurrentType = RenderNone;

}

LandscapeVisualization::~LandscapeVisualization()
{
}

void LandscapeVisualization::setup(SurfaceGraph *graph, Legend *palette)
{
    auto lg = spdlog::get("setup");
    mGraph = graph;
    mLegend = palette;
    if (!Model::hasInstance())
        return;

    if (mGraph && mGraph->graph() && mGraph->topoSeries()) {
        mGraph->graph()->removeSeries(mGraph->topoSeries());
    }

    try {
    std::string filename = Tools::path(Model::instance()->settings().valueString("visualization.dem"));
    if (!Tools::fileExists(filename)) {
        lg->error("DEM not available ('{}').", filename);
        return;
    }
    mDem.loadGridFromFile(filename);
    mMinHeight = mDem.min();
    mMaxHeight = mDem.max();

    lg->info("Loaded the DEM (visualization.dem) '{}'. Dimensions: {} x {}, with cell size: {}m. Min/max height: {}/{} ", filename, mDem.sizeX(), mDem.sizeY(), mDem.cellsize(), mMinHeight, mMaxHeight);
    lg->info("Metric rectangle with {}x{}m. Left-Right: {}m - {}m, Top-Bottom: {}m - {}m.  ", mDem.metricRect().width(), mDem.metricRect().height(), mDem.metricRect().left(), mDem.metricRect().right(), mDem.metricRect().top(), mDem.metricRect().bottom());

    graph->setup(mDem, mMinHeight, mMaxHeight);


    setupColorRamps();
    setupStateColors();
    mIsValid = true;

    } catch( const std::exception &e ) {
        lg->error("Error in setup of landscape visualization: {}", e.what());
        mIsValid = false;
    }

    connect(palette, &Legend::paletteChanged, this, &LandscapeVisualization::update);
    connect(palette, &Legend::manualValueRangeChanged, this, &LandscapeVisualization::update);

    connect(graph, &SurfaceGraph::pointSelected, this, &LandscapeVisualization::pointSelected);


}

void LandscapeVisualization::renderToFile()
{
    if (!isValid())
        return;

    QImage img = mGraph->graph()->renderToImage(0, QSize(2000, 2000));
    QString file_name = QString::fromStdString(Tools::path("render.png"));
    img.save(file_name);
    spdlog::get("main")->info("Saved image to {}", file_name.toStdString());

}

bool LandscapeVisualization::renderExpression(QString expression)
{
    if (!isValid())
        return false;

    mCurrentType = RenderExpression;

    bool auto_scale = true; // scale colors automatically between min and maximum value


    checkTexture();


    try {

        QElapsedTimer timer;
        timer.start();
        mExpression.setExpression(expression.toStdString());
        if (mExpression.isEmpty())
            return false;

        mLegend->setCaption(QString::fromStdString(mExpression.expression()));
        mLegend->setDescription("user defined expression.");
        doRenderExpression(auto_scale);

        spdlog::get("main")->info("Rendered expression '{}' ({} ms)", expression.toStdString(), timer.elapsed());

        return true;

    } catch (const std::exception &e) {
        spdlog::get("main")->error("Visualization error: {}", e.what());
        return false;
    }


}

bool LandscapeVisualization::renderVariable(QString variableName, QString description)
{
    if (!isValid())
        return false;

    mCurrentType = RenderVariable;

    bool auto_scale = true; // scale colors automatically between min and maximum value


    checkTexture();


    try {

        QElapsedTimer timer;
        timer.start();
        mExpression.setExpression(variableName.toStdString());
        if (mExpression.isEmpty())
            return false;

        mLegend->setCaption(QString::fromStdString(mExpression.expression()));
        mLegend->setDescription(description);
        doRenderExpression(auto_scale);

        spdlog::get("main")->info("Rendered variable '{}' ({} ms)", variableName.toStdString(), timer.elapsed());

        return true;

    } catch (const std::exception &e) {
        spdlog::get("main")->error("Visualization error: {}", e.what());
        return false;
    }

}


void LandscapeVisualization::update()
{
    if (!isValid() || isRendering())
        return;


    switch (mCurrentType) {
    case RenderNone: return;
    case RenderExpression: doRenderExpression(true); return;
    case RenderState: doRenderState(); return;
    case RenderVariable: doRenderExpression(true); return;
    }

}

void LandscapeVisualization::resetView(int camera)
{
    if (mGraph) {
        mGraph->resetCameraPosition(camera);

    }
}

void LandscapeVisualization::saveView(int camera)
{
    // save the camera position
    if (mGraph)
        mGraph->saveCameraPosition(camera);
}

void LandscapeVisualization::doRenderExpression(bool auto_scale)
{
    if (mExpression.isEmpty())
        return;

    mIsRendering = true;

    CellWrapper cw(nullptr);
    auto &grid = Model::instance()->landscape()->grid();
    double value;
    double min_value = 0.;
    double max_value = 1000.; // defaults

    if (auto_scale) {
        min_value = std::numeric_limits<double>::max();
        max_value = std::numeric_limits<double>::min();
        for (Cell &c : grid) {
            if (!c.isNull()) {
                cw.setData(&c);
                value = mExpression.calculate(cw);
                min_value = std::min(min_value, value);
                max_value = std::max(max_value, value);
            }
        }
    }
    mLegend->setAbsoluteValueRange(min_value, max_value);
    Palette *pal = (mLegend->currentPalette() == nullptr ? mContinuousPalette : mLegend->currentPalette() );

    //double value_rel;
    QRgb fill_color=QColor(127,127,127,127).rgba();

    const uchar *cline = mRenderTexture.scanLine(0);
    QRgb* line = reinterpret_cast<QRgb*>(const_cast<uchar*>(cline)); // write directly to the buffer (without a potential detach)

    for (int y = grid.sizeY()-1; y>=0; --y) {
        for (int x=0; x<grid.sizeX(); ++x, ++line) {
            const Cell &c = grid(x,y);
            if (!c.isNull()) {
                cw.setData(&c);
                value = mExpression.calculate(cw);
                *line = pal->color(value);
            } else {
                *line = fill_color;
            }
        }
    }
    mGraph->topoSeries()->setTexture(mRenderTexture);
    ++mRenderCount;

    spdlog::get("main")->info("Rendered expression '{}', min-value: {}, max-value: {}, Render#: {}", mExpression.expression(), min_value, max_value, mRenderCount);
    mIsRendering = false;
}

void LandscapeVisualization::doRenderState()
{
    mIsRendering = true;
    checkTexture();
    auto &grid = Model::instance()->landscape()->grid();

    mLegend->setPalette(mStatePalette);

    QRgb fill_color=QColor(255,255,255,255).rgba();

    const uchar *cline = mRenderTexture.scanLine(0);
    QRgb* line = reinterpret_cast<QRgb*>(const_cast<uchar*>(cline)); // write directly to the buffer (without a potential detach)

    for (int y = grid.sizeY()-1; y>=0; --y) {
        for (int x=0; x<grid.sizeX(); ++x, ++line) {
            const Cell &c = grid(x,y);
            if (!c.isNull()) {
                *line = mStatePalette->color(c.state()->id());
            } else {
                *line = fill_color;
            }
        }
    }
    if (mUpscaleFactor == 1) {
        mGraph->topoSeries()->setTexture(mRenderTexture);
    } else {
        mUpscaleRenderTexture = mRenderTexture.scaled(mUpscaleRenderTexture.size());
        mGraph->topoSeries()->setTexture(mRenderTexture);
    }
    ++mRenderCount;

    spdlog::get("main")->info("Rendered state, Render#: {}", mRenderCount);
    mIsRendering = false;


}

void LandscapeVisualization::checkTexture()
{
    mRenderTexture = mGraph->topoSeries()->texture();
    if (mRenderTexture.isNull() || mRenderTexture.width() != mDem.sizeX() || mRenderTexture.height() != mDem.sizeX()) {
        mRenderTexture = QImage(Model::instance()->landscape()->grid().sizeX(), Model::instance()->landscape()->grid().sizeY(),QImage::Format_ARGB32_Premultiplied);
        mUpscaleFactor = 1;
        if (Model::instance()->landscape()->grid().sizeX() != mDem.sizeX()) {
            // for upscaling to DEM size
            mUpscaleRenderTexture = QImage(mDem.sizeX(), mDem.sizeY(), QImage::Format_ARGB32_Premultiplied);
            mUpscaleFactor = mDem.sizeX() / Model::instance()->landscape()->grid().sizeX();
        }
    }

}

void LandscapeVisualization::setupColorRamps()
{
    mContinuousPalette = new Palette();

    std::vector< std::pair< float, QString> > stops = {{0.0f, "black"},
                                                       {0.33f, "blue"},
                                                       {0.67f, "red"},
                                                       {1.0f, "yellow"}};

    mContinuousPalette->setupContinuousPalette("black-yellow", stops);
    mLegend->addPalette(mContinuousPalette->name(), mContinuousPalette);

    Palette *pal = new Palette();

    stops = {{0.0f, "blue"},
             {1.0f, "red"}};

    pal->setupContinuousPalette("blue-red", stops);
    mLegend->addPalette(pal->name(), pal);

    pal = new Palette();

    stops = {{0.0f, "grey"},
             {1.0f, "red"}};

    pal->setupContinuousPalette("grey-red", stops);
    mLegend->addPalette(pal->name(), pal);

//    QVector<QColor> ColorPalette::mTerrainCol = QVector<QColor>() << QColor("#00A600") << QColor("#24B300") << QColor("#4CBF00") << QColor("#7ACC00")
//                                                           << QColor("#ADD900") << QColor("#E6E600") << QColor("#E8C727") << QColor("#EAB64E")
//                                                           << QColor("#ECB176") << QColor("#EEB99F") << QColor("#F0CFC8") <<  QColor("#F2F2F2");

    pal = new Palette();
    stops = { {0/12., "#00A600"}, {1.f/12.f, "#24B300"}, {2.f/12.f, "#4CBF00"},
              {3/12., "#7ACC00"}, {4.f/12.f, "#ADD900"}, {5.f/12.f, "#E6E600"},
              {6/12., "#E8C727"}, {7.f/12.f, "#EAB64E"}, {8.f/12.f, "#ECB176"},
              {9/12., "#EEB99F"}, {10.f/12.f, "#F0CFC8"}, {11.f/12.f, "#F2F2F2"}  };
    pal->setupContinuousPalette("terrain", stops);
    mLegend->addPalette(pal->name(), pal);

    pal = new Palette();
    stops = { {0.f, "#0080FF"}, {0.25f, "#FFFF00"}, {0.5f, "#FF8000"},
              {0.75f, "#FF0000"},
              {1.f, "#FF0080"}  };
    pal->setupContinuousPalette("rainbow", stops);
    mLegend->addPalette(pal->name(), pal);
}

void LandscapeVisualization::setupStateColors()
{
    const auto &states = Model::instance()->states()->states();

    Palette *pal = new Palette();
    QVector<QString> color_names;
    QVector<int> factor_values;
    QVector<QString> factor_labels;
    for (const auto &s : states) {
        color_names.push_back(QString::fromStdString(s.colorName()));
        factor_values.push_back(s.id());
        factor_labels.push_back(QString::fromStdString(s.asString()));
    }
    pal->setupFactorPalette("States", color_names, factor_values, factor_labels);
    mStatePalette = pal;
    mStatePalette->setDescription("state specific colors (defined in state meta data).");
    mLegend->addPalette(pal->name(), pal);

}
