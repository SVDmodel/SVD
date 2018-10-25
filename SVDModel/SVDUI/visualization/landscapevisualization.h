#ifndef LANDSCAPEVISUALIZATION_H
#define LANDSCAPEVISUALIZATION_H

#include <QString>
#include <QImage>
#include "grid.h"
#include "expression.h"
#include "colorpalette.h"

class SurfaceGraph; // forward

class LandscapeVisualization : public QObject
{
Q_OBJECT
public:
    enum RenderType {RenderNone, RenderState, RenderExpression};
    LandscapeVisualization(QObject *parent=nullptr);
    ~LandscapeVisualization();
    void setup(SurfaceGraph *graph, ColorPalette *palette);

    bool isValid() const {return mIsValid; }
    void invalidate() { mIsValid = false;  mCurrentType = RenderNone; }

    RenderType renderType() const { return mCurrentType; }
    void setRenderType(RenderType type) { mCurrentType = type; update(); }

    void renderToFile();

public slots:
    /// renders the expression and sets the render type to "Expression"
    bool renderExpression(QString expression);

    /// update the rendering
    void update();

private:
    /// render based on the result of an expression (cell variables)
    void doRenderExpression(bool auto_scale);
    /// render state colors
    void doRenderState();

    void checkTexture();
    void setupColorRamps();
    void setupStateColors();
    QRgb colorValue(double value) { return mColorLookup[ std::min(std::max(static_cast<int>(value*1000), 0), 999)]; }

    QVector<QRgb> mColorLookup;
    QVector<QRgb> mStateColorLookup;

    QImage mRenderTexture;

    SurfaceGraph *mGraph;
    Grid<float> mDem;
    float mMinHeight;
    float mMaxHeight;

    Expression mExpression;

    int mRenderCount;
    bool mIsValid;
    RenderType mCurrentType;

    ColorPalette *mPalette;

    Palette *mStatePalette;
    Palette *mContinuousPalette;

};

#endif // LANDSCAPEVISUALIZATION_H
