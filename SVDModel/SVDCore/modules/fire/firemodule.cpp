#include "firemodule.h"

#include "tools.h"
#include "model.h"
#include "filereader.h"

FireModule::FireModule()
{
}

void FireModule::setup()
{
    lg = spdlog::get("setup");
    lg->info("Setup of FireModule");
    auto settings = Model::instance()->settings();
    settings.requiredKeys("modules.fire", {"transitionFile", "stateFile", "ignitionFile", "extinguishProb"});

    // set up the transition matrix
    std::string filename = settings.valueString("modules.fire.transitionFile");
    mFireMatrix.load(Tools::path(filename));

    // set up additional fire parameter values per state
    filename = settings.valueString("modules.fire.stateFile");
    Model::instance()->states()->loadProperties(Tools::path(filename));

    // check if variables are available
    for (auto a : {"pSeverity", "pBurn"})
        if (State::valueIndex(a) == -1)
            lg->error("The FireModule requires the state property '{}' which is not available.", a);

    // set up ignitions
    filename = Tools::path(settings.valueString("modules.fire.ignitionFile"));
    FileReader rdr(filename);
    rdr.requiredColumns({"year", "x", "y", "max_size"});
    size_t iyr=rdr.columnIndex("year"), ix=rdr.columnIndex("x"), iy=rdr.columnIndex("y"), is=rdr.columnIndex("max_size");
    while (rdr.next()) {
        int year = static_cast<int>(rdr.value(iyr));
        mIgnitions.emplace(std::make_pair(year, SIgnition(year, rdr.value(ix), rdr.value(iy), rdr.value(is))));
    }
    lg->debug("Loaded {} ignitions from '{}'", mIgnitions.size(), filename);

    // set up parameters
    mExtinguishProb = settings.valueDouble("modules.fire.extinguishProb");

    // setup of the fire grid (values per cell)
    auto grid = Model::instance()->landscape()->grid();
    mGrid.setup(grid.metricRect(), grid.cellsize());
    lg->debug("Created fire grid {} x {} cells.", mGrid.sizeX(), mGrid.sizeY());

    lg->info("Setup of FireModule complete.");

    lg = spdlog::get("main");

}

int FireModule::run()
{
    // check if we have ignitions
    auto grid = Model::instance()->landscape()->grid();
    auto range = mIgnitions.equal_range(Model::instance()->year());
    int n_ignited=0;
    for (auto i=range.first; i!=range.second; ++i) {
        SIgnition &ignition = i->second;
        lg->info("FireModule: ignition at {}/{} with max-size {}.", ignition.x, ignition.y, ignition.max_size);
        if (!grid.coordValid(ignition.x, ignition.y)) {
            lg->info("Coordinates invalid. Skipping.");
            continue;
        }
        Cell &c = grid.valueAt(ignition.x, ignition.y);
        if (c.isNull()) {
            lg->info("Ignition point not in project area. Skipping.");
            continue;
        }
        ++n_ignited;
        fireSpread(ignition);
    }
    lg->info("FireModule: end of year. #ignitions: {}.", n_ignited);
    return n_ignited;
}

void FireModule::fireSpread(const FireModule::SIgnition &ign)
{
    auto grid = Model::instance()->landscape()->grid();
    Point index = grid.indexAt(PointF(ign.x, ign.y));

    // clear the spread flag for all cells
    std::for_each(mGrid.begin(), mGrid.end(), [](SFireCell &c) { c.spread=0; });

    mGrid[index].spread = 1;
    int max_ha = static_cast<int>(ign.max_size / 10000);
    int n_ha = 1;

    while (n_ha <= max_ha) {
        // TODO:
    }
}
