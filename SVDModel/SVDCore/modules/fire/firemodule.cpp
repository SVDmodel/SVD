#include "firemodule.h"

#include "tools.h"
#include "model.h"
#include "filereader.h"
#include "randomgen.h"

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
    miBurnProbability = State::valueIndex("pBurn");
    miHighSeverity = State::valueIndex("pSeverity");

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
    auto &grid = Model::instance()->landscape()->grid();
    auto range = mIgnitions.equal_range(Model::instance()->year());
    int n_ignited=0;
    for (auto i=range.first; i!=range.second; ++i) {
        SIgnition &ignition = i->second;
        lg->debug("FireModule: ignition at {}/{} with max-size {}.", ignition.x, ignition.y, ignition.max_size);
        if (!grid.coordValid(ignition.x, ignition.y)) {
            lg->debug("Coordinates invalid. Skipping.");
            continue;
        }
        Cell &c = grid.valueAt(ignition.x, ignition.y);
        if (c.isNull()) {
            lg->debug("Ignition point not in project area. Skipping.");
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
    auto &grid = Model::instance()->landscape()->grid();
    Point index = grid.indexAt(PointF(ign.x, ign.y));

    // clear the spread flag for all cells
    std::for_each(mGrid.begin(), mGrid.end(), [](SFireCell &c) { c.spread=0; });

    mGrid[index].spread = 1;
    int max_ha = static_cast<int>(ign.max_size / 10000);
    int n_ha = 0;
    int n_highseverity_ha = 0;

    int ixmin=index.x() - 1, ixmax = index.x() + 1, iymin = index.y() - 1, iymax = index.y() + 1;
    int ixmin2=ixmin, ixmax2=ixmax, iymin2=iymin, iymax2=iymax;
    int n_round, n_rounds = 1;
    while (n_ha <= max_ha) {
        n_round=0;
        for (int iy=iymin; iy<=iymax; ++iy)
            for (int ix=ixmin; ix<=ixmax; ++ix) {
                if (mGrid(ix, iy).spread == 1) {

                    if (burnCell(ix, iy, n_highseverity_ha, n_rounds)) {
                        // the cell is burning and can spread
                        if (mGrid.isIndexValid(ix-1, iy)) {
                            auto &px = mGrid[Point(ix-1, iy)];
                            if (px.spread == 0) {
                                px.spread = 1;
                                ixmin2 = std::min(ixmin, ix-1);
                            }
                        }
                        if (mGrid.isIndexValid(ix+1, iy)) {
                            auto &px = mGrid[Point(ix+1, iy)];
                            if (px.spread == 0) {
                                px.spread = 1;
                                ixmax2 = std::max(ixmax, ix+1);
                            }
                        }
                        if (mGrid.isIndexValid(ix, iy-1)) {
                            auto &px = mGrid[Point(ix, iy-1)];
                            if (px.spread == 0) {
                                px.spread = 1;
                                iymin2 = std::min(iymin, iy-1);
                            }
                        }
                        if (mGrid.isIndexValid(ix, iy+1)) {
                            auto &px = mGrid[Point(ix, iy+1)];
                            if (px.spread == 0) {
                                px.spread = 1;
                                iymax2 = std::max(iymax, iy+1);
                            }
                        }
                        n_ha++;
                        n_round++;

                    }
                }
            }
        if (n_round == 0) {
            lg->debug("Fire: stopped, no more burning pixels found.");
            break;
        }
        n_rounds++;
        ixmin = ixmin2; ixmax = ixmax2; iymin = iymin2; iymax = iymax2;
        if (lg->should_log(spdlog::level::debug))
              lg->debug("Round {}, burned: {} ha, max-size: {} ha, rectangle: {}/{}-{}/{}", n_rounds, n_ha, max_ha, ixmin, iymin, ixmax, iymax);

    } // end while
    lg->info("FireEvent. total burned (ha): {}, high severity (ha): {}, max-fire-size (ha): {}", n_ha, n_highseverity_ha, max_ha);
}


// examine a single cell and eventually burn.
bool FireModule::burnCell(int ix, int iy, int &rHighSeverity, int round)
{
    auto &grid = Model::instance()->landscape()->grid();
    auto &c = mGrid[Point(ix, iy)];
    auto &s = grid[Point(ix, iy)];
    if (s.isNull()) {
        c.spread = -1;
        return false;
    }
    // burn probability
    double pBurn = s.state()->value(miBurnProbability);
    if (round>3)
        pBurn *= (1. - mExtinguishProb);
    if (pBurn == 0. || pBurn < drandom()) {
        c.spread = -1;
        return false;
    }

    bool high_severity = drandom() < s.state()->value(miHighSeverity);
    // effect of fire: a transition to another state
    state_t new_state = mFireMatrix.transition(s.stateId(), high_severity ? 1 : 0);
    s.setNewState(new_state);
    c.last_burn = static_cast<short int>(Model::instance()->year());
    c.n_fire++;
    if (high_severity) {
        c.n_high_severity++;
        rHighSeverity++;
    }
    c.spread = 2;
    return true;
}


