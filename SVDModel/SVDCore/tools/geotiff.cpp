#include "geotiff.h"

#include <iostream>
#include <sstream>

#include "spdlog/spdlog.h"

#include "grid.h"

#include "strtools.h"
#include "third_party/FreeImage/FreeImage.h"

FIBITMAP *GeoTIFF::mProjectionBitmap = nullptr;

GeoTIFF::GeoTIFF()
{
    dib = nullptr;
    mOx = mOy = mCellsize = 0.;
    mNcol = mNrow = 0;
}

GeoTIFF::~GeoTIFF()
{
    if (dib) {
        FreeImage_Unload(dib);
        dib = nullptr;
    }
}

void GeoTIFF::clearProjection()
{
    if (mProjectionBitmap) {
        FreeImage_Unload(mProjectionBitmap);
        mProjectionBitmap = nullptr;
    }
}

int GeoTIFF::loadImage(const std::string &fileName)
{
    auto lg = spdlog::get("setup");
    lg->debug("Loading TIF file '{}'", fileName);
    dib = FreeImage_Load(FIF_TIFF, fileName.c_str());
    if (!mProjectionBitmap) {

        mProjectionBitmap = FreeImage_Allocate(10,10,24);
        FreeImage_CloneMetadata(mProjectionBitmap, dib);
    }


    unsigned int count;
    if ((count = FreeImage_GetMetadataCount(FIMD_GEOTIFF, dib))>0) {
        FITAG *tagMake = nullptr;
        FreeImage_GetMetadata(FIMD_GEOTIFF, dib, "GeoTiePoints", &tagMake);
        if (!tagMake)
            throw logic_error_fmt("GeoTIF '{}' does not contain required tags (tie points).", fileName);

        if (FreeImage_GetTagType(tagMake) != FIDT_DOUBLE)
            throw logic_error_fmt("GeoTIF '{}' invalid datatype (tie points).", fileName);

        size_t tag_count = FreeImage_GetTagCount(tagMake);
        const double *values = static_cast<const double*>(FreeImage_GetTagValue(tagMake));
        if (!values)
            throw logic_error_fmt("GeoTIF '{}' does not contain required tags (tie points).", fileName);
        for (size_t i=0; i<tag_count;++i) {
            lg->debug("TIFF: TiePoints Value {}: {}", i, values[i]);
        }
        mOx = values[0];
        mOy = values[1];
        if (mOx == 0. && mOy == 0.) {
            mOx = values[3];
            mOy = values[4];
        }


        FreeImage_GetMetadata(FIMD_GEOTIFF, dib, "GeoPixelScale", &tagMake);
        if (!tagMake)
            return -1;
        if (FreeImage_GetTagType(tagMake) != FIDT_DOUBLE)
            throw logic_error_fmt("GeoTIF '{}' does not contain required tags (pixel scale).", fileName);
        tag_count = FreeImage_GetTagCount(tagMake);
        values = static_cast<const double*>(FreeImage_GetTagValue(tagMake));
        if (!values)
            throw logic_error_fmt("GeoTIF '{}' does not contain required tags (pixel scale).", fileName);

        mCellsize = values[0];
        if (fabs(mCellsize-values[1])>0.001) {
            throw logic_error_fmt("GeoTIF '{}': pixel scale in x and y do not match (x: {}, y: {}).", fileName, mCellsize, values[1]);
        }
        mNcol = FreeImage_GetWidth(dib);
        mNrow = FreeImage_GetHeight(dib);


        lg->info("Loaded TIF '{}', x/y: {}/{}, cellsize: {}, width: {}, height: {}, datatype {}, {} bits per cell", fileName, mOx, mOy, mCellsize, mNcol, mNrow, FreeImage_GetImageType(dib), FreeImage_GetBPP(dib));
        return 0;

    } else {
        throw logic_error_fmt("GeoTIF '{}' does not contain meta data.", fileName);
    }


}

void GeoTIFF::copyToIntGrid(Grid<int> *grid)
{
    if (!dib)
        throw std::logic_error("Copy TIF to grid: tif not loaded!");
    if (FreeImage_GetImageType(dib) != FIT_INT32) {
        throw logic_error_fmt("Copy TIF to grid: wrong data type, INT32 expected, got type {}", FreeImage_GetImageType(dib));
    }
    // the null value of grids (at least for INT) is weird; it is not the smallest possible value (−2,147,483,648), but instead −2,147,483,647.
    int null_value = grid->nullValue();
    int value_null = std::numeric_limits<LONG>::min()+2;

    for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
        LONG *bits = (LONG*)FreeImage_GetScanLine(dib, y);
        for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
            grid->valueAtIndex(x,y) = bits[x] < value_null ? null_value : bits[x];
        }
    }
}

void GeoTIFF::copyToDoubleGrid(Grid<double> *grid)
{
    if (!dib)
        throw std::logic_error("Copy TIF to grid: tif not loaded!");
    if (FreeImage_GetImageType(dib) != FIT_DOUBLE && FreeImage_GetImageType(dib) != FIT_FLOAT) {
        throw std::logic_error("Copy TIF to grid: wrong data type, double or float expected!");
    }
    switch (FreeImage_GetImageType(dib)) {
    case FIT_DOUBLE: {

        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            double *bits = (double*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x];
            }
        }

    }
    case FIT_FLOAT: {
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            float *bits = (float*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x];
            }
        }

    }
    }


}

void GeoTIFF::copyToFloatGrid(Grid<float> *grid)
{
    if (!dib)
        throw std::logic_error("Copy TIF to grid: tif not loaded!");
    if (FreeImage_GetImageType(dib) != FIT_DOUBLE && FreeImage_GetImageType(dib) != FIT_FLOAT) {
        throw std::logic_error("Copy TIF to grid: wrong data type, double or float expected!");
    }
    switch (FreeImage_GetImageType(dib)) {
    case FIT_DOUBLE: {

        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            double *bits = (double*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x];
            }
        }

    }
    case FIT_FLOAT: {
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            float *bits = (float*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x];
            }
        }

    }
    }


}

bool GeoTIFF::saveToFile(const std::string &fileName)
{
    if (!dib)
        return false;
    bool success = FreeImage_Save(FIF_TIFF, dib, fileName.c_str(), TIFF_DEFAULT);
    FreeImage_Unload(dib);
    dib = nullptr;
    return success;

}

void GeoTIFF::initialize(size_t width, size_t height)
{
    if (!mProjectionBitmap)
        throw std::logic_error("GeoTif: init write: no projection information is available. You need to load at least one TIF including projection info before writing a TIF.");


    dib = FreeImage_AllocateT(FIT_FLOAT, width, height );
    FreeImage_CloneMetadata(dib, mProjectionBitmap);


}

void GeoTIFF::setValue(size_t ix, size_t iy, double value)
{
    if (!dib)
        return;
    if (ix > FreeImage_GetWidth(dib) || iy > FreeImage_GetHeight(dib))
        return;
    float flt_value = static_cast<float>(value);
    ((float*)FreeImage_GetScanLine(dib, iy))[ix] = flt_value;
}
