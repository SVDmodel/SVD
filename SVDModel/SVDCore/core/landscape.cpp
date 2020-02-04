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
#include "landscape.h"

#include "model.h"
#include "filereader.h"
#include "settings.h"
#include "tools.h"
#include "strtools.h"
#include "randomgen.h"

Landscape::Landscape()
{
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


    Grid<int> grid;
    grid.loadGridFromFile(grid_file_name);

    lg->info("Loaded the grid (landscape.grid) '{}'. Dimensions: {} x {}, with cell size: {}m. ", grid_file_name, grid.sizeX(), grid.sizeY(), grid.cellsize());
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
    // update the names of the environment:
    std::vector<std::string> &vars = EnvironmentCell::variables();
    vars.clear();
    for (size_t i=0;i<rdr.columnCount();++i) {
        if (rdr.columnName(i) != "climateId" && rdr.columnName(i)!="id")
            vars.push_back(rdr.columnName(i));
    }


    while (rdr.next()) {
        int cid = int(rdr.value(i_clim));
        int id = int(rdr.value(i_id));

        mEnvironmentCells.push_back( EnvironmentCell (id, cid) );
        EnvironmentCell &ecell=mEnvironmentCells.back();
        for (size_t i=0;i<rdr.columnCount();++i)
            if (i!=i_clim && i!=i_id)
                ecell.setValue(rdr.columnName(i), rdr.value(i));
        // store all climate regions that are present
        mClimateIds[cid]++;
    }

    lg->info("Loaded the environment file (landscape.file) '{}'.", table_file_name);
    lg->debug("Environment: added {} entries for the variables: '{}'", mEnvironmentCells.size(), join(vars, ", "));

    // setup the env-grid with the same extent:
    mEnvironmentGrid.setup(grid.metricRect(), grid.cellsize());

    // setup a hash for quick look up: do this only when cell-vector does not change anymore
    std::unordered_map<int, EnvironmentCell*> env_hash;
    for (EnvironmentCell &c : mEnvironmentCells)
        env_hash.insert(std::pair<int, EnvironmentCell*>(c.id(), &c));

    EnvironmentCell **ec=mEnvironmentGrid.begin();
    int *iptr=grid.begin();
    try {
        for (; iptr!=grid.end(); ++iptr, ++ec) {
            if (!grid.isNull(*iptr)) {
                EnvironmentCell *cell = env_hash.at(*iptr);
                *ec = cell;
            } else {
                *ec = nullptr;
            }
        }
    } catch(const std::out_of_range &) {
        lg->error("Setup of the landscape: the ID {} (found at index {}/{} of grid '{}') is not present as 'id' in the landscape file '{}'.", *iptr, grid.indexOf(iptr).x(), grid.indexOf(iptr).y(), grid_file_name, table_file_name);
        throw std::logic_error("Setup of the landscape: The ID '" + std::to_string(*iptr) + "' is invalid. Check the log for additional details.");
    }

    // load a DEM (if available)
    Grid<float> dem;
    std::string filename = Model::instance()->settings().valueString("visualization.dem","");
    if (!filename.empty()) {
        filename = Tools::path(filename);

        if (!Tools::fileExists(filename)) {
            lg->error("DEM is provided, but the file is not available ('{}').", filename);
            return;
        }
        dem.loadGridFromFile(filename);
        lg->debug("Loaded a digital elevation model (DEM) from '{}'. Cellsize: {}m, Left-Right: {}m - {}m, Top-Bottom: {}m - {}m.", filename, dem.cellsize(), dem.metricRect().left(), dem.metricRect().right(), dem.metricRect().top(), dem.metricRect().bottom());
    }


    // now set up the landscape cells
    // the grid has the same size:
    mGrid.setup(mEnvironmentGrid.metricRect(), mEnvironmentGrid.cellsize());

    Cell *a=mGrid.begin();
    int cell_index = 0;
    mNCells = 0;
    for (EnvironmentCell **ec=mEnvironmentGrid.begin(); ec!=mEnvironmentGrid.end(); ++ec, ++a, ++cell_index)
        if (*ec) {
            a->setCellIndex(cell_index);
            // set to invalid state (different from NULL which is outside of the project area)
            a->setInvalid();
            // establish link to the environment
            a->setEnvironmentCell(*ec);
            if (!dem.isEmpty()) {
                PointF p = mGrid.cellCenterPoint(cell_index);
                if (!dem.coordValid(p))
                    throw logic_error_fmt("The digital elevation model '{}' is not valid for the point {}/{} (which is within the project area)!", filename, p.x(), p.y());
                a->setElevation( dem[p] );
            }
            ++mNCells;
        }


    setupInitialState();

    lg->info("Landscape successfully set up.");
}


void Landscape::setupInitialState()
{
    auto settings = Model::instance()->settings();
    auto lg = spdlog::get("setup");

    lg->debug("Starting the setup of the initial landscape state....");

    std::string mode = settings.valueString("initialState.mode");
    std::vector<std::string> valid_modes = {"random", "file", "grid"};
    if (!contains(valid_modes, mode))
        throw std::logic_error("Key 'initialState.mode': '" + mode + "' is invalid. Valid values are: " + join(valid_modes));


    int n_affected = 0;
    if (mode == "random") {
        for (Cell *c = grid().begin(); c!=grid().end(); ++c)
            if (!c->isNull()) {
                c->setState( Model::instance()->states()->randomState().id() );
                c->setResidenceTime(static_cast<restime_t>(irandom(0,10)));
                ++n_affected;
            }
        lg->debug("Landscape states initialized randomly ({} affected cells).", n_affected);
        return;
    }

    if (mode=="file") {
        // check if keys are available:
        auto i_state = indexOf(EnvironmentCell::variables(), "initialStateId");
        auto i_restime = indexOf(EnvironmentCell::variables(), "initialResidenceTime");
        if (i_state<0 || i_restime<0)
            throw std::logic_error("Initialize landscape state: mode is 'file' and the 'landscape.file' does not contain the columns 'initialStateId' and/or 'initialResidenceTime'.");

        Cell *cell = grid().begin();
        bool error = false;
        int n_affected=0;
        for (EnvironmentCell **ec=mEnvironmentGrid.begin(); ec!=mEnvironmentGrid.end(); ++ec, ++cell)
            if (*ec) {
                state_t t= state_t( (*ec)->value(static_cast<size_t>(i_state)) );
                short int res_time = static_cast<short int> ( (*ec)->value(static_cast<size_t>(i_restime)) );
                if (!Model::instance()->states()->isValid(t)) {
                    if (!error) lg->error("Initalize states from landscape file '{}': Errors detected:", settings.valueString("landscape.file"));
                    error = true;
                    lg->error("State: {} not valid (at {}/{})", t, grid().indexOf(cell).x(), grid().indexOf(cell).y());
                } else {
                    cell->setState(t);
                    cell->setResidenceTime(res_time);
                    ++n_affected;
                }
            }
        if (error) {
            throw std::logic_error("Initalize states from file: invalid states! Check the log for details.");
        }
        lg->debug("Landscape states initialized from file ({} affected cells).", n_affected);

    }

    if (mode=="grid") {
        // load grid from file
        std::string grid_file = Tools::path(settings.valueString("initialState.stateGrid"));
        std::string restime_grid_file = Tools::path(settings.valueString("initialState.residenceTimeGrid"));
        if (!Tools::fileExists(grid_file))
            throw std::logic_error("Initialize landscape state: mode is 'grid',  but the grid file '" + grid_file + "' ('initialState.stateGrid') does not exist.");
        if (!Tools::fileExists(restime_grid_file))
            throw std::logic_error("Initialize landscape state: mode is 'grid',  but the grid file '" + restime_grid_file + "' ('initialState.residenceTimeGrid') does not exist.");

        Grid<int> state_grid;
        state_grid.loadGridFromFile(grid_file);
        lg->debug("Loaded initial *state* grid '{}'. Dimensions: {} x {}, with cell size: {}m. ", grid_file, state_grid.sizeX(), state_grid.sizeY(), state_grid.cellsize());
        lg->debug("Metric rectangle with {}x{}m. Left-Right: {}m - {}m, Top-Bottom: {}m - {}m.  ", state_grid.metricRect().width(), state_grid.metricRect().height(), state_grid.metricRect().left(), state_grid.metricRect().right(), state_grid.metricRect().top(), state_grid.metricRect().bottom());

        Grid<int> restime_grid;
        restime_grid.loadGridFromFile(restime_grid_file);
        lg->debug("Loaded initial *residenceTime* grid '{}'. Dimensions: {} x {}, with cell size: {}m. ", restime_grid_file, restime_grid.sizeX(), restime_grid.sizeY(), restime_grid.cellsize());
        lg->debug("Metric rectangle with {}x{}m. Left-Right: {}m - {}m, Top-Bottom: {}m - {}m.  ", restime_grid.metricRect().width(), restime_grid.metricRect().height(), restime_grid.metricRect().left(), restime_grid.metricRect().right(), restime_grid.metricRect().top(), restime_grid.metricRect().bottom());

        int n_affected=0;
        int n_errors=0;
        for (int i=0;i<grid().count(); ++i) {
            if (!grid()[i].isNull()) {
                PointF p = grid().cellCenterPoint(i);
                if (!state_grid.coordValid(p) || !restime_grid.coordValid(p)) {
                    ++n_errors;
                    if (n_errors<100)
                        lg->error("Init landscape: cell with index '{}' ({}/{}) not valid in state or residence time grid.", i, p.x(), p.y());
                } else {
                    int intstate = state_grid.valueAt(p);
                    if (intstate == state_grid.nullValue()) {
                        ++n_errors;
                        if (n_errors<120)
                            lg->error("Init landscape: NA at {}/{}: not a valid stateId.",  p.x(), p.y());
                        continue;
                    }
                    state_t state = static_cast<state_t>(state_grid.valueAt(p));
                    restime_t restime = static_cast<restime_t>(restime_grid.valueAt(p));
                    if (!Model::instance()->states()->isValid(state)) {
                        ++n_errors;
                        if (n_errors<120) // make sure we get at least some of those errors....
                            lg->error("Init landscape: state '{}' (at {}/{}) is not a valid stateId.", state, p.x(), p.y());
                    } else {
                        grid()[i].setResidenceTime(restime);
                        grid()[i].setState(state);
                        ++n_affected;
                    }
                }
            }
        }
        if (n_errors>0)
            throw std::logic_error("Error in setting up the initial landscape scape (from grid). Check the log.");
        lg->debug("Initial landscape setup finished, {} cells affected.", n_affected);

    } // mode==grid


}
