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
    void setup(SurfaceGraph *graph, Legend *palette);

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
    bool isRendering() {return mIsRendering; }
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
    QImage mUpscaleRenderTexture;
    int mUpscaleFactor;

    SurfaceGraph *mGraph;
    Grid<float> mDem;
    float mMinHeight;
    float mMaxHeight;

    Expression mExpression;

    int mRenderCount;
    bool mIsValid;
    bool mIsRendering;
    RenderType mCurrentType;

    Legend *mLegend;

    Palette *mStatePalette;
    Palette *mContinuousPalette;

};

#endif // LANDSCAPEVISUALIZATION_H
