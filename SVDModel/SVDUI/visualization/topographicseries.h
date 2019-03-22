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

#ifndef TOPOGRAPHICSERIES_H
#define TOPOGRAPHICSERIES_H

#include <QObject>
#include <QtDataVisualization/QSurface3DSeries>

#include "grid.h"

//using namespace QtDataVisualization;

class TopographicSeries : public QtDataVisualization::QSurface3DSeries
{
private:
    Q_OBJECT
public:
    explicit TopographicSeries();
    ~TopographicSeries();

    void setTopographyFile(const QString file, float width, float height);

    // WR
    void setGrid(const Grid<float> &grid, float min_value);
    // convert relative coords [-1,1] to metric coordinates on the DEM
    QVector3D getCoordsFromRelative(const QVector3D &rel);

    float sampleCountX() { return m_sampleCountX; }
    float sampleCountZ() { return m_sampleCountZ; }

public Q_SLOTS:

private:
    RectF m_gridRect;
    float m_sampleCountX;
    float m_sampleCountZ;
};

#endif // TOPOGRAPHICSERIES_H
