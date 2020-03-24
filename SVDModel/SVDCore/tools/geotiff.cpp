#include "geotiff.h"

#include <iostream>
#include <sstream>

#include <QtDebug>

#include "FreeImage.h"

GeoTIFF::GeoTIFF()
{
    mProjectionBitmap = nullptr;
}

int GeoTIFF::loadImage(const std::string &fileName)
{
    FIBITMAP *dib = FreeImage_Load(FIF_TIFF, fileName.c_str());
    if (!mProjectionBitmap) {

        mProjectionBitmap = FreeImage_Allocate(10,10,24);
        FreeImage_CloneMetadata(mProjectionBitmap, dib);
    }

    double minx, miny;
    double cellsize;

    unsigned int count;
    if ((count = FreeImage_GetMetadataCount(FIMD_GEOTIFF, dib))>0) {
        FITAG *tagMake = nullptr;
        FreeImage_GetMetadata(FIMD_GEOTIFF, dib, "GeoTiePoints", &tagMake);
        if (!tagMake)
            return -1;
        if (FreeImage_GetTagType(tagMake) != FIDT_DOUBLE)
            return -1;
        size_t tag_count = FreeImage_GetTagCount(tagMake);
        const double *values = static_cast<const double*>(FreeImage_GetTagValue(tagMake));
        if (!values)
            return -1;
        for (size_t i=0; i<tag_count;++i) {
            qDebug() << "Double Params Value" << i << ":" << values[i];
        }
        minx = values[0];
        miny = values[1];
        if (minx == 0. && miny == 0.) {
            minx = values[3];
            miny = values[4];
        }


        FreeImage_GetMetadata(FIMD_GEOTIFF, dib, "GeoPixelScale", &tagMake);
        if (!tagMake)
            return -1;
        if (FreeImage_GetTagType(tagMake) != FIDT_DOUBLE)
            return -1;
        tag_count = FreeImage_GetTagCount(tagMake);
        values = static_cast<const double*>(FreeImage_GetTagValue(tagMake));
        if (!values)
            return -1;
        for (size_t i=0; i<tag_count;++i) {
            qDebug() << "PixelScale Value" << i << ":" << values[i];
        }
        cellsize = values[0];
        if (fabs(cellsize-values[1])>0.00001) {
            qDebug() << "cellsize not equal in x and y direction!";
        }
        size_t width = FreeImage_GetWidth(dib);
        size_t height = FreeImage_GetHeight(dib);

        qDebug() << "TIF dimensions: x " << minx << "y" << miny << "cellsize:" << cellsize << "width" << width << "height" << height;
        return 0;

    } else {
        qDebug() << "TIFF does not contain meta data!!!";
        return -1;
    }


}
