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
    mContinuousPalette = nullptr;
    mRenderCount = 0;
    mIsValid = false;
    mIsRendering = false;
    mCurrentType = RenderNone;
    mBGColor = QColor(127,127,127,127);

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

void LandscapeVisualization::renderToFile(QString filename)
{
    if (!isValid())
        return;

    QImage img = mGraph->graph()->renderToImage(8, QSize(1500, 1000));
    QString file_name;
    if (filename.isEmpty())
        file_name = QString::fromStdString(Tools::path("render.png"));
    else
        file_name = QString::fromStdString(Tools::path(filename.toStdString()));

    img.save(file_name);
    spdlog::get("main")->info("Saved image to {}", file_name.toStdString());

}

int LandscapeVisualization::viewCount() const
{
    if (mGraph)
        return mGraph->cameraCount();
    else
        return 0;
}

bool LandscapeVisualization::isViewValid(int camera)
{
    if (mGraph)
        return mGraph->isCameraValid(camera);
    else
        return false;
}

QString LandscapeVisualization::viewString(int camera)
{
    if (mGraph)
        return mGraph->cameraString(camera);
    else
        return QString();
}

void LandscapeVisualization::setViewString(int camera, QString str)
{
    if (mGraph)
        mGraph->setCameraString(camera, str);
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

void LandscapeVisualization::setFillColor(QColor col)
{
    mBGColor = col;
    update();
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

    QRgb fill_color=mBGColor.rgba();

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
    if (mUpscaleFactor == 1) {
        mGraph->topoSeries()->setTexture(mRenderTexture);
    } else {
        mUpscaleRenderTexture = mRenderTexture.scaled(mUpscaleRenderTexture.size());
        mGraph->topoSeries()->setTexture(mUpscaleRenderTexture);
    }
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

    QRgb fill_color=mBGColor.rgba();

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
        mGraph->topoSeries()->setTexture(mUpscaleRenderTexture);
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
            spdlog::get("main")->info("DEM Visualization uses an factor of {}", mUpscaleFactor);
        }
    }

}

void LandscapeVisualization::setupColorRamps()
{
    // only set up once
    if (mContinuousPalette != nullptr)
        return;


    mContinuousPalette = new Palette();

    // the Google "Turbo" palette
    // https://gist.github.com/mikhailov-work/6a308c20e494d9e0ccc29036b28faa7a
    // https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
    int turbo_srgb_bytes[256][3] = {{48,18,59},{50,21,67},{51,24,74},{52,27,81},{53,30,88},{54,33,95},{55,36,102},{56,39,109},{57,42,115},{58,45,121},{59,47,128},{60,50,134},{61,53,139},{62,56,145},{63,59,151},{63,62,156},{64,64,162},{65,67,167},{65,70,172},{66,73,177},{66,75,181},{67,78,186},{68,81,191},{68,84,195},{68,86,199},{69,89,203},{69,92,207},{69,94,211},{70,97,214},{70,100,218},{70,102,221},{70,105,224},{70,107,227},{71,110,230},{71,113,233},{71,115,235},{71,118,238},{71,120,240},{71,123,242},{70,125,244},{70,128,246},{70,130,248},{70,133,250},{70,135,251},{69,138,252},{69,140,253},{68,143,254},{67,145,254},{66,148,255},{65,150,255},{64,153,255},{62,155,254},{61,158,254},{59,160,253},{58,163,252},{56,165,251},{55,168,250},{53,171,248},{51,173,247},{49,175,245},{47,178,244},{46,180,242},{44,183,240},{42,185,238},{40,188,235},{39,190,233},{37,192,231},{35,195,228},{34,197,226},{32,199,223},{31,201,221},{30,203,218},{28,205,216},{27,208,213},{26,210,210},{26,212,208},{25,213,205},{24,215,202},{24,217,200},{24,219,197},{24,221,194},{24,222,192},{24,224,189},{25,226,187},{25,227,185},{26,228,182},{28,230,180},{29,231,178},{31,233,175},{32,234,172},{34,235,170},{37,236,167},{39,238,164},{42,239,161},{44,240,158},{47,241,155},{50,242,152},{53,243,148},{56,244,145},{60,245,142},{63,246,138},{67,247,135},{70,248,132},{74,248,128},{78,249,125},{82,250,122},{85,250,118},{89,251,115},{93,252,111},{97,252,108},{101,253,105},{105,253,102},{109,254,98},{113,254,95},{117,254,92},{121,254,89},{125,255,86},{128,255,83},{132,255,81},{136,255,78},{139,255,75},{143,255,73},{146,255,71},{150,254,68},{153,254,66},{156,254,64},{159,253,63},{161,253,61},{164,252,60},{167,252,58},{169,251,57},{172,251,56},{175,250,55},{177,249,54},{180,248,54},{183,247,53},{185,246,53},{188,245,52},{190,244,52},{193,243,52},{195,241,52},{198,240,52},{200,239,52},{203,237,52},{205,236,52},{208,234,52},{210,233,53},{212,231,53},{215,229,53},{217,228,54},{219,226,54},{221,224,55},{223,223,55},{225,221,55},{227,219,56},{229,217,56},{231,215,57},{233,213,57},{235,211,57},{236,209,58},{238,207,58},{239,205,58},{241,203,58},{242,201,58},{244,199,58},{245,197,58},{246,195,58},{247,193,58},{248,190,57},{249,188,57},{250,186,57},{251,184,56},{251,182,55},{252,179,54},{252,177,54},{253,174,53},{253,172,52},{254,169,51},{254,167,50},{254,164,49},{254,161,48},{254,158,47},{254,155,45},{254,153,44},{254,150,43},{254,147,42},{254,144,41},{253,141,39},{253,138,38},{252,135,37},{252,132,35},{251,129,34},{251,126,33},{250,123,31},{249,120,30},{249,117,29},{248,114,28},{247,111,26},{246,108,25},{245,105,24},{244,102,23},{243,99,21},{242,96,20},{241,93,19},{240,91,18},{239,88,17},{237,85,16},{236,83,15},{235,80,14},{234,78,13},{232,75,12},{231,73,12},{229,71,11},{228,69,10},{226,67,10},{225,65,9},{223,63,8},{221,61,8},{220,59,7},{218,57,7},{216,55,6},{214,53,6},{212,51,5},{210,49,5},{208,47,5},{206,45,4},{204,43,4},{202,42,4},{200,40,3},{197,38,3},{195,37,3},{193,35,2},{190,33,2},{188,32,2},{185,30,2},{183,29,2},{180,27,1},{178,26,1},{175,24,1},{172,23,1},{169,22,1},{167,20,1},{164,19,1},{161,18,1},{158,16,1},{155,15,1},{152,14,1},{149,13,1},{146,11,1},{142,10,1},{139,9,2},{136,8,2},{133,7,2},{129,6,2},{126,5,2},{122,4,3}};
    std::vector<std::pair<float, QString> > turbo_grad;
    for (int i=0;i<256;++i) {
        QColor c(turbo_srgb_bytes[i][0], turbo_srgb_bytes[i][1], turbo_srgb_bytes[i][2]);
        turbo_grad.push_back(std::pair<float, QString>(i/256.f, c.name()));
    }
    mContinuousPalette->setupContinuousPalette("turbo", turbo_grad);
    mLegend->addPalette(mContinuousPalette->name(), mContinuousPalette);


    Palette *pal = new Palette();
    std::vector< std::pair< float, QString> > stops = {{0.0f, "black"},
                                                       {0.33f, "blue"},
                                                       {0.67f, "red"},
                                                       {1.0f, "yellow"}};

    pal->setupContinuousPalette("black-yellow", stops);
    mLegend->addPalette(pal->name(), pal);

    pal = new Palette();

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

    // 100 distinct colors generated with R package randomcoloR
    const QStringList default_cols = {"#C6F363","#559769","#EDC1CA","#58C3B4","#6E54E4","#E57BF1","#98EE6D","#D59D8F","#A3CED6","#E2D072","#3D6152","#3FBBD1","#C28FA7","#4F83EF","#749BA9",
                               "#A663E5","#BDF0BB","#5D4C71","#69CD93","#E04D7B","#E7359A","#E6C896","#EC5EEE","#56C0ED","#D4EF86","#A79D55","#8FDBF0","#A57EBF","#ED6FD4","#EDCAAF",
                               "#F5ACC9","#8FE734","#F5EE38","#E2AA60","#BE77E5","#E7ECA2","#58EE4F","#6DEFEC","#5B8FDE","#50DDB8","#6F9329","#E6BAEE","#A0C949","#E4BF3C","#F07375",
                               "#50F6DC","#A7ED92","#E196EF","#ECEFDA","#EFEC73","#E8EABD","#BBC2AB","#BBA6A8","#EB3ECE","#8E7CEA","#829BD0","#CDE3F4","#969C83","#DAF1EB","#97663D",
                               "#EE345E","#B0A3ED","#97F1B4","#7D35A5","#DC813A","#EE6BB0","#EEE3E5","#5EEB90","#CCEF2F","#5ECC5C","#8EC19D","#63B6F1","#B0BBD5","#A7EDEB","#E27AAB",
                               "#CED847","#982EEE","#99F1D6","#C0EDD7","#404191","#C73BAF","#E84C36","#56F6C1","#E28C72","#A08CB1","#C7C5F2","#54A4A7","#BA38CA","#AF5895","#4586AD",
                               "#D1787E","#9ABFEC","#61E1F5","#7B615D","#E79FE0","#EAD1EE","#7BBC6A","#C353F1","#B4CE89","#C7CBCC"};

    Palette *pal = new Palette();
    QVector<QString> color_names;
    QVector<int> factor_values;
    QVector<QString> factor_labels;
    for (const auto &s : states) {
        if (s.colorName().empty())
            color_names.push_back( default_cols[color_names.length() % default_cols.size()] );
        else
            color_names.push_back(QString::fromStdString(s.colorName()));
        factor_values.push_back(s.id());
        factor_labels.push_back(QString::fromStdString(s.asString()));
    }
    pal->setupFactorPalette("States", color_names, factor_values, factor_labels);
    mStatePalette = pal;
    mStatePalette->setDescription("state specific colors (defined in state meta data).");
    mLegend->addPalette(pal->name(), pal);

}
