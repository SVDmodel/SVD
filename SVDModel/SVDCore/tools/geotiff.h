#ifndef GEOTIFF_H
#define GEOTIFF_H

#include <string>

//#include "grid.h"

struct FIBITMAP;
template <typename T>
class Grid; // forward

class GeoTIFF
{
public:
    GeoTIFF();
    ~GeoTIFF();

    static void clearProjection();

    int loadImage(const std::string &fileName);

    void copyToIntGrid(Grid<int> *grid);
    void copyToDoubleGrid(Grid<double> *grid);
    void copyToFloatGrid(Grid<float> *grid);

    // write Grid to file + free memory
    bool saveToFile(const std::string &fileName);
    // create a bitmap with the the size of the full grid
    void initialize(size_t width, size_t height);
    void setValue(size_t ix, size_t iy, double value);


    // getters
    double ox() { return mOx; }
    double oy() { return mOy; }
    double cellsize() { return mCellsize; }
    size_t ncol() { return mNcol; }
    size_t nrow() { return mNrow; }
private:
    static FIBITMAP *mProjectionBitmap;
    FIBITMAP *dib;

    double mOx, mOy;
    double mCellsize;
    size_t mNcol, mNrow;
};

#endif // GEOTIFF_H
