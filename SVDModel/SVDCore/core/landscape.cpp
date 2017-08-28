#include "landscape.h"

#include "model.h"
#include "filereader.h"
#include "settings.h"
#include "tools.h"
#include "gisgrid.h"
#include "strtools.h"

Landscape::Landscape()
{
    mCurrentState = &mStateA;
    mFutureState = &mStateB;
}

void Landscape::setup()
{
    auto settings = Model::instance()->settings();
    auto lg = spdlog::get("setup");

    settings.requiredKeys("landscape", {"grid", "file"});
    std::string table_file_name = Tools::path(settings.valueString("landscape.file"));
    std::string grid_file_name = Tools::path(settings.valueString("landscape.grid"));

    if (!Tools::fileExists(table_file_name))
        throw std::logic_error("Landscape setup: '" + table_file_name + "' (landscape.file) does not exist!");

    if (!Tools::fileExists(grid_file_name))
        throw std::logic_error("Landscape setup: '" + grid_file_name + "' (landscape.grid) does not exist!");


    GisGrid<int> grid;
    grid.loadFromFile(grid_file_name);

    lg->info("Loaded the grid '{}'. Dimensions: {} x {}, with cell size: {}m. Min-value: '{}', max-value: '{}'. ", grid_file_name, grid.sizeX(), grid.sizeY(), grid.cellsize(), grid.minValue(), grid.maxValue());
    lg->info("Metric rectangle with {}x{}m. Left-Right: {}m - {}m, Top-Bottom: {}m - {}m.  ", grid.metricRect().width(), grid.metricRect().height(), grid.metricRect().left(), grid.metricRect().right(), grid.metricRect().top(), grid.metricRect().bottom());
    if (lg->should_log(spdlog::level::trace)) {
        // some statistics:
        lg->trace("The grid contains '{}' not-null values.", grid.countNotNull());
        std::set<int> uval = grid.uniqueValues();
        lg->trace("Unique values: {}", join(uval.begin(), uval.end(), ",", 1000));

    }

    FileReader rdr(table_file_name);
    rdr.requiredColumns({"id", "climateId"});
    auto i_clim = rdr.columnIndex("climateId");
    auto i_id = rdr.columnIndex("id");

    while (rdr.next()) {

    }

}

void Landscape::switchStates()
{
    LandscapeState *ms = mCurrentState;
    mCurrentState = mFutureState;
    mFutureState = ms;
}
