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

    //graph->topoSeries()->setGrid(mDem, mMinHeight);

    setupColorRamps();
    setupStateColors();
    mIsValid = true;

    } catch( const std::exception &e ) {
        lg->error("Error in setup of landscape visualization: {}", e.what());
        mIsValid = false;
    }

    connect(palette, &Legend::paletteChanged, this, &LandscapeVisualization::update);
    connect(palette, &Legend::manualValueRangeChanged, this, &LandscapeVisualization::update);


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


void LandscapeVisualization::update()
{
    if (!isValid() || isRendering())
        return;


    switch (mCurrentType) {
    case RenderNone: return;
    case RenderExpression: doRenderExpression(true); return;
    case RenderState: doRenderState();
    }

}

void LandscapeVisualization::doRenderExpression(bool auto_scale)
{
    if (mExpression.isEmpty())
        return;

    mIsRendering = true;

    //mPalette->setCaption(QString::fromStdString(mExpression.expression()));
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
        //if (min_value == max_value)
        //    max_value = min_value + 1.; // avoid div by 0 later
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
                //value_rel = (value - min_value) / (max_value - min_value);
                //*line = colorValue(value_rel);
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

    QRgb fill_color=QColor(127,127,127,127).rgba();

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
    mGraph->topoSeries()->setTexture(mRenderTexture);
    ++mRenderCount;

    spdlog::get("main")->info("Rendered state, Render#: {}", mRenderCount);
    mIsRendering = false;


}

void LandscapeVisualization::checkTexture()
{
    mRenderTexture = mGraph->topoSeries()->texture();
    if (mRenderTexture.isNull() || mRenderTexture.width() != mDem.sizeX() || mRenderTexture.height() != mDem.sizeX()) {
        mRenderTexture = QImage(mDem.sizeX(), mDem.sizeY(), QImage::Format_ARGB32_Premultiplied);
    }

}

void LandscapeVisualization::setupColorRamps()
{
    mContinuousPalette = new Palette();

    std::vector< std::pair< float, QString> > stops = {{0.0, "black"},
                                                       {0.33, "blue"},
                                                       {0.67, "red"},
                                                       {1.0, "yellow"}};

    mContinuousPalette->setupContinuousPalette("black-yellow", stops);
    mLegend->addPalette(mContinuousPalette->name(), mContinuousPalette);

    Palette *pal = new Palette();

    stops = {{0.0, "blue"},
             {1.0, "red"}};

    pal->setupContinuousPalette("blue-red", stops);
    mLegend->addPalette(pal->name(), pal);

    pal = new Palette();

    stops = {{0.0, "grey"},
             {1.0, "red"}};

    pal->setupContinuousPalette("grey-red", stops);
    mLegend->addPalette(pal->name(), pal);


//    QLinearGradient gr(0,0,1000,1);

//    gr.setColorAt(0.0, Qt::black);
//    gr.setColorAt(0.33, Qt::blue);
//    gr.setColorAt(0.67, Qt::red);
//    gr.setColorAt(1.0, Qt::yellow);
//    //gr.setCoordinateMode(QGradient::StretchToDeviceMode);
//    QBrush brush(gr);
//    QImage color_map(1000, 10, QImage::Format_ARGB32_Premultiplied);

//    QPainter painter(&color_map);
//    painter.setBrush(brush);
//    painter.drawRect(color_map.rect());
//    //painter.fillRect(color_map.rect(), brush);
//    painter.end();

//    mColorLookup.clear();
//    for (int i=0;i<1000;++i)
//        mColorLookup.push_back(color_map.pixel(i,1));
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

//    int max_state_id=0;
//    for (const auto &s : states)
//        max_state_id = std::max(max_state_id, static_cast<int>(s.id()));

//    mStateColorLookup.clear();
//    mStateColorLookup.resize(max_state_id + 1);

//    QRgb no_value = QColor(100,100,100).rgba();
//    for (const auto &s : states) {
//        mStateColorLookup[static_cast<int>(s.id())] = no_value;
//        if (!s.colorName().empty()) {
//            QColor col(QString::fromStdString(s.colorName()));
//            if (col.isValid())
//                mStateColorLookup[static_cast<int>(s.id())] = col.rgba();
//            else
//                spdlog::get("main")->warn("The color '{}' for stateId '{}' is not valid (see Qt doc for valid color names)!", s.colorName(), s.id());
//        }

//    }

}
