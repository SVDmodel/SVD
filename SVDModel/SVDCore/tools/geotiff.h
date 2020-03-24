#ifndef GEOTIFF_H
#define GEOTIFF_H

#include <string>

struct FIBITMAP;
class GeoTIFF
{
public:
    GeoTIFF();
    int loadImage(const std::string &fileName);
private:
    FIBITMAP *mProjectionBitmap;
};

#endif // GEOTIFF_H
