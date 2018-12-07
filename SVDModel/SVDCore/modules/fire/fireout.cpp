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
#include "fireout.h"
#include "firemodule.h"
#include "tools.h"

#include "model.h"

FireOut::FireOut()
{
    setName("Fire");
    setDescription("Output on fire events (one event per line) and grids for the year of the last burn.\n\n" \
                   "Grids are saved as ASCII grids to the location specified by the" \
                   "`lastFireGrid.path` property (`$year$` is replaced with the actual year). " \
                   "The value of the grid cells is the year of the last burn in a cell or 0 for unburned cells.\n\n" \
                   "### Parameters\n" \
                   " * `lastFireGrid.filter`: a grid is written only if the expression evaluates to `true` (with `year` as variable). A value of 0 deactivates the grid output.");
    // define the columns
    columns() = {
    {"year", "simulation year of the fire event", DataType::Int},
    {"x", "x coordinate (m) of the ignition point", DataType::Double},
    {"y", "y coordinate (m) of the ignition point", DataType::Double},
    {"planned_size", "planned fire size (ha)", DataType::Double},
    {"realized_size", "realized fire size (ha)", DataType::Double},
    {"share_high_severity", "share of pixels burning with high severity (0..1)", DataType::Double}   };
}

void FireOut::setup()
{
    mLastFire.setExpression(Model::instance()->settings().valueString(key("lastFireGrid.filter")));
    mLastFirePath = Tools::path(Model::instance()->settings().valueString(key("lastFireGrid.path")));
    openOutputFile();


}

void FireOut::execute()
{
    FireModule *fire = dynamic_cast<FireModule*>(Model::instance()->module("fire"));
    if (!fire)
        return;

    // write output table
    for (auto &s : fire->mStats) {
        if (s.year == Model::instance()->year()) {
            out() << s.year << s.x << s.y << s.max_size << s.ha_burned << (s.ha_burned>0? s.ha_high_severity/static_cast<double>(s.ha_burned) : 0. );
            out().write();
        }
    }

    // write output grids
    if (mLastFire.calculateBool( Model::instance()->year() )) {
        std::string file_name = mLastFirePath;
        find_and_replace(file_name, "$year$", to_string(Model::instance()->year()));
        auto &grid = fire->mGrid;

        std::string result = gridToESRIRaster<SFireCell>(grid, [](const SFireCell &c) { return std::to_string(c.last_burn); });
        if (!writeFile(file_name, result))
            throw std::logic_error("FireOut: couldn't write output grid file file: " + file_name);


    }
}
