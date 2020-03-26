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
#include "grid.h"
//#include "exception.h"
//#include "global.h"
#include <string>
#include "geotiff.h"

struct SCoordTrans {
    SCoordTrans() { setupTransformation(0.,0.,0.,0.); }
    double RotationAngle;
    double sinRotate, cosRotate;
    double sinRotateReverse, cosRotateReverse;
    double offsetX, offsetY, offsetZ;
    void setupTransformation(double new_offsetx, double new_offsety, double new_offsetz, double angle_degree)
    {
#ifndef M_PI
#define M_PI 3.141592653589793
#endif
        offsetX = new_offsetx;
        offsetY = new_offsety;
        offsetZ = new_offsetz;
        RotationAngle=angle_degree * M_PI / 180.;
        sinRotate=sin(RotationAngle);
        cosRotate=cos(RotationAngle);
        sinRotateReverse=sin(-RotationAngle);
        cosRotateReverse=cos(-RotationAngle);
    }
} GISCoordTrans;

// setup of global GIS transformation
// not a good place to put that code here.... please relocate!
void setupGISTransformation(double offsetx, double offsety, double offsetz, double angle_degree)
{
    GISCoordTrans.setupTransformation(offsetx, offsety, offsetz, angle_degree);
}

void worldToModel(const Vector3D &From, Vector3D &To)
{
    double x=From.x() - GISCoordTrans.offsetX;
    double y=From.y() - GISCoordTrans.offsetY;
    To.setZ( From.z() - GISCoordTrans.offsetZ );
    To.setX( x * GISCoordTrans.cosRotate - y*GISCoordTrans.sinRotate);
    To.setY( x * GISCoordTrans.sinRotate + y*GISCoordTrans.cosRotate);
    //To.setY(-To.y()); // spiegeln
}
void modelToWorld(const Vector3D &From, Vector3D &To)
{
    double x=From.x();
    double y=From.y(); // spiegeln
    To.setX( x * GISCoordTrans.cosRotateReverse - y*GISCoordTrans.sinRotateReverse + GISCoordTrans.offsetX);
    To.setY( x * GISCoordTrans.sinRotateReverse + y*GISCoordTrans.cosRotateReverse  + GISCoordTrans.offsetY);
    To.setZ( From.z() + GISCoordTrans.offsetZ );
}

PointF modelToWorld(PointF model_coordinates)
{
    Vector3D to;
    modelToWorld(Vector3D(model_coordinates.x(), model_coordinates.y(), 0.), to);
    return PointF(to.x(), to.y());
}

PointF worldToModel(PointF world_coordinates)
{
    Vector3D to;
    worldToModel(Vector3D(world_coordinates.x(), world_coordinates.y(), 0.), to);
    return PointF(to.x(), to.y());

}


std::string gridToString(const DoubleGrid &grid, const char sep, const int newline_after)
{

    std::string res;

    int newl_counter = newline_after;
    for (int y=grid.sizeY()-1;y>=0;--y) {
         for (int x=0;x<grid.sizeX();x++) {

             res+= std::to_string(grid(x,y)) + sep;
            if (--newl_counter==0) {
                res += "\n";
                newl_counter = newline_after;
            }
        }
        res+="\r\n";
    }
    return res;
}



#ifdef ILAND_GUI
#include <QImage>

QImage gridToImage(const FloatGrid &grid,
                   bool black_white,
                   double min_value, double max_value,
                   bool reverse)
{
    QImage res(grid.sizeX(), grid.sizeY(), QImage::Format_ARGB32);
    QRgb col;
    int grey;
    double rval;
    for (int x=0;x<grid.sizeX();x++){
        for (int y=0;y<grid.sizeY();y++) {
            rval = grid.constValueAtIndex(QPoint(x,y));
            rval = std::max(min_value, rval);
            rval = std::min(max_value, rval);
            if (reverse) rval = max_value - rval;
            if (black_white) {
                grey = int(255 * ( (rval-min_value) / (max_value-min_value)));
                col = QColor(grey,grey,grey).rgb();
            } else {
                col = QColor::fromHsvF(0.66666666666*rval, 0.95, 0.95).rgb();
            }
            res.setPixel(x,y,col);
            //res+=QString::number(grid.constValueAtIndex(QPoint(x,y))) + ";";
        }
        //res+="\r\n";
    }
    return res;
}



bool loadGridFromImage(const QString &fileName, FloatGrid &rGrid)
{
    QImage image;
    if (!image.load(fileName))
        throw IException(QString("Grid::loadFromImage: could not load image file %1.").arg(fileName));
    if (rGrid.isEmpty())
        rGrid.setup(1., image.size().width(), image.size().height() );
    double value;
    for (int x=0;x<image.width(); x++)
        for (int y=0;y<image.height(); y++) {
            value = qGray(image.pixel(x,y))/255.;
            if (rGrid.isIndexValid(QPoint(x,y)))
                rGrid.valueAtIndex(x,y) = value;
        }
    return true;
}
#else
//QImage gridToImage(const FloatGrid &grid,
//                   bool black_white,
//                   double min_value, double max_value,
//                   bool reverse)
//{
//}

//bool loadGridFromImage(const QString &fileName, FloatGrid &rGrid) {
//    Q_UNUSED(fileName); Q_UNUSED(rGrid);
//    throw IException("Error: trying to load a grid from an image in the console version (GUI version only!)");
//    return false;
//}
#endif

