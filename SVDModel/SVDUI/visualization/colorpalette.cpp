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
#include "colorpalette.h"
#include <QVector>
#include <QGradient>
#include <QPainter>
//#include "dem.h"

#include "spdlog/spdlog.h"

Legend *Legend::mInstance = nullptr;

QVector<QColor> ColorPalette::mBrewerDiv = QVector<QColor>() << QColor("#543005") << QColor("#8c510a") << QColor("#bf812d") << QColor("#dfc27d")
                                                       << QColor("#f6e8c3") << QColor("#f5f5f5") << QColor("#fdbf6f") << QColor("##c7eae5")
                                                       << QColor("#80cdc1") << QColor("#35978f") << QColor("#01665e") <<  QColor("#003c30");

QVector<QColor> ColorPalette::mBrewerQual = QVector<QColor>() << QColor("#a6cee3") << QColor("#1f78b4") << QColor("#b2df8a") << QColor("#33a02c")
                                                       << QColor("#fb9a99") << QColor("#e31a1c") << QColor("#fdbf6f") << QColor("#ff7f00")
                                                       << QColor("#cab2d6") << QColor("#6a3d9a") << QColor("#ffff99") <<  QColor("#b15928");


QVector<QColor> ColorPalette::mTerrainCol = QVector<QColor>() << QColor("#00A600") << QColor("#24B300") << QColor("#4CBF00") << QColor("#7ACC00")
                                                       << QColor("#ADD900") << QColor("#E6E600") << QColor("#E8C727") << QColor("#EAB64E")
                                                       << QColor("#ECB176") << QColor("#EEB99F") << QColor("#F0CFC8") <<  QColor("#F2F2F2");

void ColorPalette::setPalette(const GridViewType type, const float min_val, const float max_val)
{
    if (mNeedsPaletteUpdate==false && type==mCurrentType &&
            (mAutoScale==false || (minValue()==min_val && maxValue()==max_val))  )
        return;

    mHasFactors = false;
    int n = 50;
    if (type >= GridViewBrewerDiv) {
        // categorical values...
        mHasFactors = true;
        n=mFactorLabels.size();
        if (mFactorLabels.isEmpty()) {
            n=max_val;
            mFactorLabels.clear();
            for (int i=0;i<n;++i)
                mFactorLabels.append(QString("Label %1").arg(i));
        }
    }
    if (type != GridViewCustom) {
        mColors.clear();
        for (int i=0;i<n;++i)
            if (mHasFactors)
                mColors.append(colorFromValue(i, type, 0., 1.).name());
            else
                mColors.append(colorFromValue(1. - i/double(n), type, 0., 1.).name());

    }
    mLabels = QStringList() << QString::number(min_val)
                            << QString::number((3.*min_val + max_val)/4.)
                            << QString::number((min_val+max_val)/2.)
                            << QString::number((min_val + 3.*max_val)/4.)
                            << QString::number(max_val);

    if (mAutoScale) {
        mMinValue = min_val;
        mMaxValue = max_val;
    }
    mCurrentType = type;
    mNeedsPaletteUpdate = false;
    emit colorsChanged();
}

void ColorPalette::setFactorLabels(QStringList labels)
{
    mFactorLabels = labels;
    mNeedsPaletteUpdate = true;
}

ColorPalette::ColorPalette(QObject *parent): QObject(parent)
{
    mNeedsPaletteUpdate =true;
    mAutoScale = true;
    mHasFactors = false;
    mMeterPerPixel = 1.;
    //default start palette
    //setPalette(GridViewRainbow, 0, 1);
    // factors test
    setCaption("");
    setPalette(GridViewTerrain, 0, 4);
}

QColor ColorPalette::colorFromPalette(const int value, const GridViewType view_type)
{
    if (value<0)
        return Qt::white;
    int n = qMax(value,0) % 12;
    QColor col;
    switch(view_type) {
    case GridViewBrewerDiv: col = mBrewerDiv[n]; break;
    case GridViewBrewerQual: col = mBrewerQual[n]; break;
    case GridViewTerrain: col = mTerrainCol[n]; break;
    default: return QColor();
    }
    if (value < 12)
        return col;
    n = qMax(value,0) % 60;
    if (n<12) return col;
    if (n<24) return col.darker(200);
    if (n<36) return col.lighter(150);
    if (n<48) return col.darker(300);
    return col.lighter(200);

}



// colors
QColor ColorPalette::colorFromValue(const float value,
                              const float min_value, const float max_value,
                              const bool reverse, const bool black_white)
{
    float rval = value;
    rval = std::max(min_value, rval);
    rval = std::min(max_value, rval);
    if (reverse)
        rval = max_value - rval;
    float rel_value;
    QColor col;
    if (min_value < max_value) {
        // default: high values -> red (h=0), low values to blue (h=high)
        rel_value = 1 - (rval - min_value) / (max_value - min_value);
        if (black_white) {
            int c = (1.-rel_value)*255;
            col = QColor(c,c,c);
        } else
            col=  QColor::fromHsvF(0.66666666666*rel_value, 0.95, 0.95).rgb();
    } else {
        col = Qt::white;
    }
    return col;
}

QColor ColorPalette::colorFromValue(const float value, const GridViewType view_type, const float min_value, const float max_value)
{
    if (view_type==GridViewGray || view_type==GridViewGrayReverse)
        return colorFromValue(value, min_value, max_value, view_type==GridViewGrayReverse, true);

    if (view_type==GridViewRainbow || view_type==GridViewRainbowReverse)
        return colorFromValue(value, min_value, max_value, view_type==GridViewRainbowReverse, false);

    if (view_type == GridViewGreens || view_type==GridViewBlues || view_type==GridViewReds) {
        float rval = value;
        rval = std::max(min_value, rval);
        rval = std::min(max_value, rval);
        float rel_value = (max_value!=min_value)?(rval - min_value) / (max_value - min_value): 0;
        int r,g,b;
        switch (view_type) {
        case GridViewGreens:  // 11,111,19
            r=220 - rel_value*(220-11); g=220-rel_value*(220-111); b=220-rel_value*(220-19); break;
        case GridViewBlues: //15,67,138
            r=220 - rel_value*(220-15); g=220-rel_value*(220-67); b=220-rel_value*(220-138); break;
        case GridViewReds: //219,31,72
            r=240 - rel_value*(220-219); g=240-rel_value*(220-31); b=240-rel_value*(220-72); break;
        default: r=g=b=0;
        }
        return QColor(r,g,b);

    }
    if (view_type == GridViewHeat) {
        float rval = value;
        rval = std::max(min_value, rval);
        rval = std::min(max_value, rval);
        float rel_value = 1 - (rval - min_value) / (max_value - min_value);
        int g=255, b=0;
        if (rel_value < 0.5)
            g = rel_value*2.f * 255;
        if (rel_value>0.5)
            b = (rel_value-0.5)*2.f * 255;
        return QColor(255,g,b);

    }
    return colorFromPalette(value, view_type);


}

void Palette::setupContinuousPalette(QString name, std::vector<std::pair<float, QString> > &gradient_def)
{
    mIsFactor = false;
    mName = name;

    QLinearGradient gr(5,0, 5,1000);
    for (auto &def : gradient_def) {
        QColor col = QColor(def.second);
        if (!col.isValid()) {
            spdlog::get("main")->warn("The color '{}' for palette '{}' is not valid (see Qt doc for valid color names)!", def.second.toStdString(), mName.toStdString());
            col = Qt::gray;
        }
        float p = def.first;
        if (p<0. || p>1.) {
            spdlog::get("main")->warn("The value '{}' for palette '{}' is not a valid stop (0..1 allowed)!", p, mName.toStdString());
            p = 1.;
        }
        gr.setColorAt(1. - p, col);
    }

    QBrush brush(gr);
    QImage color_map(10, 1000, QImage::Format_ARGB32_Premultiplied);

    QPainter painter(&color_map);
    painter.setBrush(brush);
    painter.fillRect(color_map.rect(), brush);
    painter.end();

    //color_map.save("e:/temp/color_map.png");

    mLegendImage = color_map;

    mColorLookup.clear();
    for (int i=0;i<1000;++i)
        mColorLookup.push_back(color_map.pixel(1,999-i));

    mIsValid = true;

}

void Palette::setupFactorPalette(QString name, QVector<QString> color_names, QVector<int> factor_values, QVector<QString> factor_labels)
{
    mIsFactor = true;
    mName = name;
    if (!(color_names.size() == factor_labels.size() && factor_labels.size() == factor_values.size() && factor_labels.size()>0)) {
        spdlog::get("main")->warn("Not the same number of factor levels for palette '{}' !", mName.toStdString());
        return;
    }

    int max_factor=0;
    for (int s : factor_values)
        max_factor = std::max(max_factor, s);

    if (max_factor > 100000)
        spdlog::get("main")->warn("Very high factor levels for palette '{}' - check code !", mName.toStdString());

    QRgb invalid_col = qRgba(127,127,127,127);
    mColorLookup.clear();
    for (int i=0;i<max_factor+1;++i) {
        mColorLookup.push_back(invalid_col);
    }

    for (int i=0;i<color_names.size();++i) {
        QColor col = QColor(color_names[i]);
        if (!col.isValid()) {
            spdlog::get("main")->warn("The color '{}' for palette '{}' is not valid (see Qt doc for valid color names)!", color_names[i].toStdString(), mName.toStdString());
            col = Qt::gray;
        }
        mFactorLabels.push_back(factor_labels[i]);
        mFactorColors.push_back(color_names[i]);
        // the lookup has a different range (optimized for fast access)
        int factor = factor_values[i];
        mColorLookup[factor] = col.rgba();
    }
    mIsValid = true;
}

Legend::~Legend()
{
    for (int i=0;i<mPalettes.size();++i)
        delete mPalettes[i];
    mPalettes.clear();
}

void Legend::addPalette(QString name, Palette *palette)
{
    if (!palette->isFactor()) {
        mPaletteNames.push_back(name);
        mPalettes.push_back(palette);

        // set the first added palette as the current
        if (currentPalette() == &mEmptyPalette) {
            mCurrent = palette;
            mPaletteIndex = 0;
            emit paletteChanged();
        }

        emit namesChanged();
    }
}

Palette *Legend::palette(QString name)
{
    int idx = mPaletteNames.indexOf(name);
    if (idx>-1)
        return mPalettes[idx];
    return nullptr;
}

void Legend::setPalette(Palette *new_pal)
{
    if (mCurrent != new_pal) {
        mCurrent = new_pal;
        emit paletteChanged();
        setCaption(new_pal->name());
        setDescription(new_pal->description());
    }
}

void Legend::setContinuous()
{
    if (mCurrent->isFactor()) {
        // switch to a continuous palette
        int cur_pal = mPaletteIndex;
        mPaletteIndex++; // force setting a new palette by changing this
        setPaletteIndex(cur_pal);
    }
}

void Legend::setPaletteIndex(int new_index)
{
    if (new_index != mPaletteIndex) {
        if (new_index>=0 && new_index<mPalettes.size())
            mCurrent = palette(new_index);
        else
            mCurrent = &mEmptyPalette;

        mPaletteIndex = new_index;
        currentPalette()->setValueRange(mMinValue, mMaxValue);

        emit paletteChanged();
    }
}

void Legend::updateRangeLabels()
{
    mRangeLabels = QStringList() << QString::number(mMinValue)
                            << QString::number((3.*mMinValue + mMaxValue)/4.)
                            << QString::number((mMinValue+mMaxValue)/2.)
                            << QString::number((mMinValue + 3.*mMaxValue)/4.)
                            << QString::number(mMaxValue);
    if (currentPalette())
        currentPalette()->setValueRange(mMinValue, mMaxValue);

}

QImage ColorImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)

    Palette *pal = Legend::instance()->palette(id);
    if (pal) {
        QImage img = pal->legendImage();
        if (size && size->isValid())
            return img.scaled(*size);
        else
            return img;
    }
    return QImage();
}


