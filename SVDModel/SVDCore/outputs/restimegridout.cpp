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
