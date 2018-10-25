/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
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
#ifndef COLORPALETTE_H
#define COLORPALETTE_H
#include <QObject>
#include <QColor>
#include <QImage>
#include "grid.h"
/** Colors: helper class for managing/selecting colors
 *
 * */
class DEM; // forward


class Palette: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QImage legendImage READ legendImage)
    Q_PROPERTY(QStringList factorLabels READ factorLabels)
    Q_PROPERTY(QVector<QColor> factorColors READ factorColors)
    Q_PROPERTY(bool valid READ isValid)

public:
    Palette(QObject *parent=nullptr) : QObject(parent), mIsFactor(false), mIsValid(false), mMinValue(0.), mMaxValue(1.), mAbsMinValue(0.), mAbsMaxValue(1.) {}

    /// setup for continuous palette
    /// the gradient definition comes as pairs of position (0..1) and color name
    void setupContinuousPalette(QString name, std::vector< std::pair< float, QString> > &gradient_def);

    /// setup for factor palette
    /// a list of color names, factor values (int) and labels
    void setupFactorPalette(QString name, QVector<QString> color_names, QVector<int> factor_values, QVector<QString> factor_labels);

    // properties
    /// the name of the color palette
    QString name() { return mName; }
    /// returns true if the palette has factor levels, false if it continuous
    bool isFactor() { return mIsFactor; }
    /// returns true if the palette is properly set up
    bool isValid() { return mIsValid; }

    QImage legendImage() { return mLegendImage; }

    QStringList factorLabels() { return mFactorLabels; }
    QVector<QColor> factorColors() {return mFactorColors; }

    // actions

    /// set the absolute range of the data (global min/max)
    void setAbsolutValueRange(double min_value, double max_value) { mAbsMinValue = min_value; mMinValue = min_value; mAbsMaxValue = max_value; mMaxValue = max_value; if(min_value==max_value) mMaxValue+=1.; }
    /// set value range (min/max) for a user-defined range
    void setValueRange(double min_value, double max_value) { mMinValue = min_value; mMaxValue = max_value; if(min_value==max_value) mMaxValue+=1.; }

    /// get the corresponding color of `value` with the current settings of the palette
    QRgb color(double value) { assert(isValid()); double value_rel = (value - mMinValue) / (mMaxValue - mMinValue);
                               return mColorLookup[ std::min(std::max(static_cast<int>(value_rel*1000), 0), 999)]; }
    QRgb color(int factor_value) { assert(isValid() && factor_value>=0 && factor_value<mColorLookup.size()); return mColorLookup[factor_value];  }
private:
    QString mName;
    bool mIsFactor;
    bool mIsValid;
    QImage mLegendImage;

    // for continues: 1000 values, for factors: colors from 0..max_factor_value
    QVector<QRgb> mColorLookup;

    QStringList mFactorLabels;
    QVector<QColor> mFactorColors;


    double mMinValue;
    double mMaxValue;

    double mAbsMinValue;
    double mAbsMaxValue;


};


class ColorPalettes: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ isValid)
public:
    ColorPalettes(QObject *parent=nullptr): QObject(parent), mCurrent(nullptr) {}
    ~ColorPalettes();
    void addPalette(QString name, Palette *palette);
    Palette *palette(QString name);
    Palette *palette(int index) { assert(index>=0 && index<mPalettes.count()); return mPalettes[index];}

    Palette *currentPalette() { return mCurrent; }
    bool isValid() {return mCurrent != nullptr; }
private:
    QVector<Palette*> mPalettes;
    QStringList mPaletteNames;
    Palette *mCurrent;

};


class ColorPalette: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList colors READ colors NOTIFY colorsChanged)
    Q_PROPERTY(QStringList labels READ labels NOTIFY colorsChanged)
    Q_PROPERTY(QStringList factorLabels READ factorLabels NOTIFY colorsChanged)
    Q_PROPERTY(int count READ colorCount NOTIFY colorsChanged)
    Q_PROPERTY(double minValue READ minValue WRITE setMinValue NOTIFY colorsChanged)
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue NOTIFY colorsChanged)
    Q_PROPERTY(bool autoScale READ autoScale WRITE setAutoScale NOTIFY colorsChanged)
    Q_PROPERTY(bool hasFactors READ hasFactors NOTIFY colorsChanged)
    Q_PROPERTY(QString caption READ caption NOTIFY colorsChanged)
    Q_PROPERTY(QString description READ description NOTIFY colorsChanged)
    Q_PROPERTY(double meterPerPixel READ meterPerPixel NOTIFY scaleChanged)



public:
    ColorPalette(QObject *parent=nullptr);
    // properties
    QStringList colors() const {return mColors; }
    QStringList labels() const {return mLabels; }
    QStringList factorLabels() const {return mFactorLabels; }
    int colorCount() const { return mColors.count(); }
    double minValue() const {return mMinValue; }
    double maxValue() const {return mMaxValue; }
    void setMinValue(double val) { if(val==mMinValue) return;
        mNeedsPaletteUpdate=true; setPalette(mCurrentType, val, mMaxValue); mMinValue = val; }
    void setMaxValue(double val) { if(val==mMaxValue) return;
        mNeedsPaletteUpdate=true; setPalette(mCurrentType, mMinValue, val); mMaxValue = val; }
    bool hasFactors() const { return mHasFactors; }
    bool autoScale() const {return mAutoScale; }
    void setAutoScale(bool value) { if (value==mAutoScale) return; mAutoScale=value; mNeedsPaletteUpdate=true; setPalette(mCurrentType, mMinValue, mMaxValue);}
    QString caption() const {return mCaption; }
    QString description() const {return mDescription; }

    void setPalette(const GridViewType type, const float min_val, const float max_val);
    void setFactorLabels(QStringList labels);
    void setFactorColors(QStringList colors) { mColors = colors; }
    void setCaption(QString caption, QString description=QString()) {
        if (mCaption==caption && mDescription==description) return;
        mCaption = caption; mDescription=description;mNeedsPaletteUpdate=true; }

    // scale
    double meterPerPixel() const {return mMeterPerPixel; }
    void setScale(double meter_per_pixel) { if(mMeterPerPixel==meter_per_pixel) return;
                                             mMeterPerPixel = meter_per_pixel; emit scaleChanged();}

    static QColor colorFromValue(const float value, const float min_value=0.f, const float max_value=1.f, const bool reverse=false, const bool black_white=false);
    static QColor colorFromValue(const float value, const GridViewType view_type, const float min_value=0.f, const float max_value=1.f);
    static QColor colorFromPalette(const int value, const GridViewType view_type);
private:
    static QVector<QColor> mBrewerDiv;
    static QVector<QColor> mBrewerQual;
    static QVector<QColor> mTerrainCol;
    QStringList mColors;
    QStringList mLabels;
    QStringList mFactorLabels;
    double mMinValue;
    double mMaxValue;
    GridViewType mCurrentType;
    bool mAutoScale;
    bool mHasFactors;
    bool mNeedsPaletteUpdate;
    QString mCaption;
    QString mDescription;
    double mMeterPerPixel;
signals:
    void colorsChanged();
    void scaleChanged();
};

#endif // COLORPALETTE_H
