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
#include "restimegridout.h"
#include "model.h"
#include "tools.h"

ResTimeGridOut::ResTimeGridOut()
{
    setName("ResTimeGrid");
    setDescription("output of ASCII grids with the residence time (years) for each cell.\n" \
                   "Grids are save to the location specified with the `path`property "
                   "(`$year$` is replaced with the actual year).\n" \
                   "### Parameters\n" \
                   "* `interval`: output is written only every `interval` years (or every year if `interval=0`). For example, a value of 10 limits output to the simulation years 1, 11, 21, ...\n");
    mInterval=0;

}

void ResTimeGridOut::setup()
{
    auto lg = spdlog::get("setup");
    mInterval = Model::instance()->settings().valueInt(key("interval"));
    mPath = Tools::path(Model::instance()->settings().valueString(key("path")));
    lg->debug("Setup of ResTimeGrid output, set interval to {}, path to: {}.", mInterval, mPath);

}

void ResTimeGridOut::execute()
{
    int year =  Model::instance()->year();
    if (mInterval>0)
        if (year % mInterval != 1)
            return;
    // write grids...
    std::string file_name = mPath;
    find_and_replace(file_name, "$year$", to_string(year));
    auto &grid = Model::instance()->landscape()->grid();
    std::string result = gridToESRIRaster<Cell>(grid, [](const Cell &c) { if (c.isNull()) return std::string("-9999"); else return std::to_string(c.residenceTime()); });
    if (!writeFile(file_name, result))
        throw std::logic_error("ResTimeGridOut: couldn't write output file: " + file_name);

}
