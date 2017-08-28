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


#ifndef GISGRID_H
#define GISGRID_H

#include <set>

#include "grid.h"
#include "math.h"
#include "strtools.h"


struct SCoordTrans {
    SCoordTrans() { setupTransformation(0.,0.,0.,0.); }
    double RotationAngle;
    double sinRotate, cosRotate;
    double sinRotateReverse, cosRotateReverse;
    double offsetX, offsetY, offsetZ;
    void setupTransformation(double new_offsetx, double new_offsety, double new_offsetz, double angle_degree)
    {
        const double M_PI = 3.141592653589793;
        offsetX = new_offsetx;
        offsetY = new_offsety;
        offsetZ = new_offsetz;
        RotationAngle=angle_degree * M_PI / 180.;
        sinRotate=sin(RotationAngle);
        cosRotate=cos(RotationAngle);
        sinRotateReverse=sin(-RotationAngle);
        cosRotateReverse=cos(-RotationAngle);
    }
};

/** GIS Transformation
    The transformation is defined by three offsets (x,y,z) and a rotation information.
    the (x/y) denotes the real-world coordinates of the left lower edge of the grid (at least for the northern hemisphere), i.e. the (0/0).
    The z-offset is currently not used.

  */
// setup routine -- give a vector with offsets [m] and rotation angle.
void setupGISTransformation(const double offsetx,
                            const double offsety,
                            const double offsetz,
                            const double angle_degree);
// transformation routines.
void worldToModel(const Vector3D &From, Vector3D &To);
void modelToWorld(const Vector3D &From, Vector3D &To);
PointF modelToWorld(PointF model_coordinates);
PointF worldToModel(PointF world_coordinates);


template <typename T>
class GisGrid: public Grid<T> {
public:
    bool loadFromFile(const std::string &fileName); ///< load ESRI style text file
    // global conversion functions between local and world coordinates
    // the world-context is created by calling setupGISTransformation() (once).
    /// convert model to world coordinates (metric)
    static PointF modelToWorld(PointF model_coordinates);
    /// convert world (i.e. GIS) to model coordinates (metric) (with 0/0 at lower left edge of project area)
    static PointF worldToModel(PointF world_coordinates);

    // special properties
    T minValue() const { return mMinValue; }
    T maxValue() const { return mMaxValue; }
    /// retrieve all unique non-null values
    std::set<T> uniqueValues() const;
    /// count of cells which are not null
    int countNotNull();


private:
    T mMaxValue;
    T mMinValue;
    PointF mOrigin; // lowerleftcorner
    PointF xAxis;  // transformed axis (moved, rotated)
    PointF yAxis;
    T mNODATAValue;
};

class GisGrid2
{
public:
    GisGrid2();
    GisGrid2(const std::string &fileName) { loadFromFile(fileName); }
    ~GisGrid2();
    // maintenance
    bool loadFromFile(const std::string &fileName); ///< load ESRI style text file
    // access
    int dataSize() const { return mDataSize; }   ///< number of data items (rows*cols)
    int rows() const { return mNRows; } ///< number of rows
    int cols() const { return mNCols; } ///< number of columns
    PointF origin() const { return mOrigin; } ///< coordinates of the lower left corner of the grid
    double cellSize() const { return mCellSize; } ///< size of a cell (meters)
    double minValue() const { return min_value; } ///< minimum data value
    double maxValue() const { return max_value; } ///< maximum data value
    int noDataValue() const { return mNODATAValue; } ///< no data value of the grid
    /// get grid value at local coordinates (X/Y); returs NODATAValue if out of range
    /// @p X and @p Y are local coordinates.
    double value(const PointF &p) const {return value(p.x(), p.y());}
    double value(const double X, const double Y) const;
    double value(const int indexx, const int indexy) const; ///< get value of grid at index positions
    double value(const int Index) const; ///< get value of grid at index positions
    /// get coordinates of the center of the cell with 'Index'
    Vector3D coord(const int Index) const;
    Vector3D coord(const int indexx, const int indexy) const;
    RectF rectangle(const int indexx, const int indexy) const;
    void clip(const RectF & box); ///< clip the grid to 'Box' (set values outside to -1)

    // special operations
    //RectF GetBoundingBox(int LookFor, SBoundingBox &Result, double x_m, double y_m);
    //QList<double> distinctValues(); ///< returns a list of distinct values
    //void GetDistinctValues(TStringList *ResultList, double x_m, double y_m);
    //void CountOccurence(int intID, int & Count, int & left, int &upper, int &right, int &lower, SBoundingBox *OuterBox);
    //S3dVector GetNthOccurence(int ID, int N, int left, int upper, int right, int lower);
    //void ExportToTable(AnsiString OutFileName);

    // global conversion functions between local and world coordinates
    // the world-context is created by calling setupGISTransformation() (once).
    /// convert model to world coordinates (metric)
    static PointF modelToWorld(PointF model_coordinates);
    /// convert world (i.e. GIS) to model coordinates (metric) (with 0/0 at lower left edge of project area)
    static PointF worldToModel(PointF world_coordinates);


private:

    int mDataSize;  ///< number of data items (rows*cols)
    double mCellSize;   // size of cells [m]
    double max_value;
    double min_value;
    PointF mOrigin; // lowerleftcorner
    PointF xAxis;  // transformed axis (moved, rotated)
    PointF yAxis;
    int mNRows;
    int mNCols;     // count of rows and cols
    double *mData;
    int mNODATAValue;
};


template <typename T>
bool GisGrid<T>::loadFromFile(const std::string &fileName)
{
    mMinValue = std::numeric_limits<T>::max();
    mMaxValue = std::numeric_limits<T>::min();

    std::vector<std::string> lines = readFile(fileName);
    std::vector<std::string>::iterator l = lines.begin();

    bool in_header=true;
    std::string key;
    std::string strValue;
    T value;
    double cell_size;
    int n_rows, n_cols;
    do {
        if (l == lines.end())
            throw std::logic_error(std::string("GisGrid: unexpected end of file ") + fileName);

        size_t str_pos = (*l).find(" ");
        key = (*l).substr(0, str_pos);

        if (!isalpha(key[0])) {
            in_header=false; // we reachted datalines
        } else {
            strValue = (*l).substr(str_pos+1);
            value = atof(strValue.c_str());
            if (key=="ncols")
                n_cols=int(value);
            else if (key=="nrows")
                n_rows=int(value);
            else if (key=="xllcorner")
                mOrigin.setX(value);
            else if (key=="yllcorner")
                mOrigin.setY(value);
            else if (key=="cellsize")
                cell_size = value;
            else if (key=="NODATA_value")
                mNODATAValue=value;
            else
                throw std::logic_error(std::string("GisGrid: invalid key ") + key);
            ++l;
        }
    } while (in_header);

    // create the underlying grid

    RectF rect(mOrigin.x(), mOrigin.y(), mOrigin.x() + n_cols*cell_size, mOrigin.y() + n_rows*cell_size );
    setup(rect, cell_size);
    //setup(cell_size, n_cols, n_rows);


    // loop thru datalines
    int i,j;
    const char *p=0;
    const char *p2;

    --l;
    for (i=1;i<=n_rows;++i)
        for (j=0;j<n_cols;j++) {
            // copy next value to buffer, change "," to "."
            if (!p || *p==0) {
                ++l;
                if (l==lines.end())
                    throw std::logic_error("TGisGrid: Unexpected End of File!");
                p=(*l).c_str();
                // replace chars
                p2=p;
                while (*p2) {
                    if (*p2==',')
                        *const_cast<char*>(p2)='.';
                    p2++;
                }
            }
            // skip spaces
            while (*p && strchr(" \r\n\t", *p))
                p++;
            if (*p) {
                value = atof(p);
                if (value!=mNODATAValue) {
                    mMinValue=std::min(mMinValue, value);
                    mMaxValue=std::max(mMaxValue, value);
                }
                valueAtIndex(j, n_rows - i) = value;
                // skip text...
                while (*p && !strchr(" \r\n\t", *p))
                    p++;
            } else
                j--;
        }


    return true;
}

template<typename T>
std::set<T> GisGrid<T>::uniqueValues() const
{
    std::set<T> unique_values;
    for (T* p=begin(); p!=end(); ++p) {
        if (*p != mNODATAValue)
            unique_values.insert(*p);
    }
    return unique_values;
}

template<typename T>
int GisGrid<T>::countNotNull()
{
    int result = 0;
    for (T* p=begin(); p!=end(); ++p)
        if (*p != mNODATAValue)
            ++result;

    return result;
}


//---------------------------------------------------------------------------
#endif
