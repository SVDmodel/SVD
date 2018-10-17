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
