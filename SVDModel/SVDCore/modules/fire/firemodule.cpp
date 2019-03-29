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
#include "firemodule.h"

#include "tools.h"
#include "model.h"
#include "filereader.h"
#include "randomgen.h"

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

FireModule::FireModule(std::string module_name): Module(module_name, State::None)
{

}

void FireModule::setup()
{
    lg = spdlog::get("setup");
    lg->info("Setup of FireModule '{}'", name());
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
            throw logic_error_fmt("The FireModule requires the state property '{}' which is not available.", a);

    miBurnProbability = static_cast<size_t>(State::valueIndex("pBurn"));
    miHighSeverity = static_cast<size_t>(State::valueIndex("pSeverity"));

    // check if DEM is available
    if (settings.valueString("visualization.dem").empty())
        throw logic_error_fmt("The fire module requires a digital elevation model! {}", 0);

    // set up ignitions
    filename = Tools::path(settings.valueString("modules.fire.ignitionFile"));
    FileReader rdr(filename);
    rdr.requiredColumns({"year", "x", "y", "max_size", "windspeed", "winddirection"});
    size_t iyr=rdr.columnIndex("year"), ix=rdr.columnIndex("x"), iy=rdr.columnIndex("y"), is=rdr.columnIndex("max_size"), iws=rdr.columnIndex("windspeed"), iwd=rdr.columnIndex("winddirection");
    size_t iid = rdr.columnIndex("id");
    int fire_id = 0;
    while (rdr.next()) {
        int year = static_cast<int>(rdr.value(iyr));
        fire_id = iid!=std::string::npos ? static_cast<int>(rdr.value(iid)) : fire_id + 1; // use provided Ids or a custom Id
        mIgnitions.emplace(std::make_pair(year, SIgnition(year, fire_id, rdr.value(ix), rdr.value(iy), rdr.value(is), rdr.value(iws), rdr.value(iwd))));
    }
    lg->debug("Loaded {} ignitions from '{}'", mIgnitions.size(), filename);

    // set up parameters
    mExtinguishProb = settings.valueDouble("modules.fire.extinguishProb");
    mSpreadToDistProb = 1. - settings.valueDouble("modules.fire.spreadDistProb");

    // setup of the fire grid (values per cell)
    auto grid = Model::instance()->landscape()->grid();
    mGrid.setup(grid.metricRect(), grid.cellsize());
    lg->debug("Created fire grid {} x {} cells.", mGrid.sizeX(), mGrid.sizeY());

    lg->info("Setup of FireModule '{}' complete.", name());

    lg = spdlog::get("modules");

}

std::vector<std::pair<std::string, std::string> > FireModule::moduleVariableNames() const
{
    return {{"fireSpread", "progress of the last fire (value is the iteration)"},
        {"fireNFires", "cumulative number of fires"},
        {"fireNHighSeverity", "cumulative number of high severity fires"},
        {"fireLastBurn", "the year of the last fire on a cell (or 0 if never burned)"}};
}

double FireModule::moduleVariable(const Cell *cell, size_t variableIndex) const
{
    auto &gr = mGrid[cell->cellIndex()];
    switch (variableIndex) {
    case 0:
        return gr.spread;
    case 1:
        return gr.n_fire;
    case 2:
        return gr.n_high_severity;
    case 3:
        return gr.last_burn;
    default:
        return 0.;
    }
}

void FireModule::run()
{
    // check if we have ignitions
    auto &grid = Model::instance()->landscape()->grid();
    auto range = mIgnitions.equal_range(Model::instance()->year());
    int n_ignited=0;
    for (auto i=range.first; i!=range.second; ++i) {
        SIgnition &ignition = i->second;
        lg->debug("FireModule: ignition at {}/{} with max-size {} ha.", ignition.x, ignition.y, ignition.max_size);
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

    // fire output
    Model::instance()->outputManager()->run("Fire");

}


void FireModule::fireSpread(const FireModule::SIgnition &ign)
{
    auto &grid = Model::instance()->landscape()->grid();
    Point index = grid.indexAt(PointF(ign.x, ign.y));

    // clear the spread flag for all cells
    std::for_each(mGrid.begin(), mGrid.end(), [](SFireCell &c) { c.spread=0.f; });

    mGrid[index].spread = 1.f; // initial value
    int max_ha = static_cast<int>(ign.max_size);
    int n_ha = 0;
    int n_highseverity_ha = 0;
    int grid_max_x = grid.sizeX()-1, grid_max_y=grid.sizeY()-1;

    int ixmin=std::max(index.x() - 1,0);
    int ixmax = std::min(index.x() + 1, grid_max_x);
    int iymin = std::max(index.y() - 1, 0);
    int iymax = std::min(index.y() + 1, grid_max_y);

    int ixmin2=ixmin, ixmax2=ixmax, iymin2=iymin, iymax2=iymax;
    int n_burned_in_round, n_rounds = 1;

    if (!burnCell(index.x(), index.y(), n_highseverity_ha, n_rounds)) {
        lg->debug("Fire: not spreading, stopped at ignition point.");
    } else {
        ++n_ha; // one cell already burned
        while (n_ha <= max_ha) {
            n_burned_in_round=0;
            // calculate spread probabilities based on wind and slope from currently burning px
            for (int iy=iymin; iy<=iymax; ++iy)
                for (int ix=ixmin; ix<=ixmax; ++ix) {
                    if (mGrid(ix, iy).spread == 1.f) {
                        // value = 1.f -> the cell burned and is spreading
                        float elev_origin = Model::instance()->landscape()->grid()(ix,iy).elevation();
                        // direction codes: (1..8, N, E, S, W, NE, SE, SW, NW)
                        calculateSpreadProbability(ign, Point(ix-1, iy+1), elev_origin, 8); // NW
                        calculateSpreadProbability(ign, Point(ix  , iy+1), elev_origin, 1); // N
                        calculateSpreadProbability(ign, Point(ix+1, iy+1), elev_origin, 5); // NE
                        calculateSpreadProbability(ign, Point(ix+1, iy  ), elev_origin, 2); // E
                        calculateSpreadProbability(ign, Point(ix+1, iy-1), elev_origin, 6); // SE
                        calculateSpreadProbability(ign, Point(ix  , iy-1), elev_origin, 3); // S
                        calculateSpreadProbability(ign, Point(ix-1, iy-1), elev_origin, 7); // SW
                        calculateSpreadProbability(ign, Point(ix-1, iy  ), elev_origin, 4); // W
                        // the cell has spread, mark the iteration
                        mGrid.valueAtIndex(ix, iy).spread = static_cast<float>(n_rounds + 1);
                    }
                }
            for (int iy=iymin; iy<=iymax; ++iy) {
                for (int ix=ixmin; ix<=ixmax; ++ix) {
                    float &p_spread = mGrid.valueAtIndex(ix, iy).spread;
                    if (p_spread < 1.f && p_spread > 0.f) {
                        // the cell is spreading, calculate the probability and decide using a random number
                        if (drandom() < p_spread) {
                            if (burnCell(ix, iy, n_highseverity_ha, n_rounds)) {
                                // the cell really burned, potentially increase the bounding box
                                ixmin2 = std::max(std::min(ixmin2, ix-1), 0);
                                ixmax2 = std::min(std::max(ixmax2, ix+1), grid_max_x);
                                iymin2 = std::max(std::min(iymin2, iy-1), 0);
                                iymax2 = std::min(std::max(iymax2, iy+1), grid_max_y);
                                n_ha++;
                                n_burned_in_round++;
                            } else {
                                p_spread = 0.; // did not burn, reset
                            }
                        } else {
                            p_spread = 0.; // did not burn, reset
                        }
                    }
                }
            }

            if (n_burned_in_round == 0) {
                lg->debug("Fire: stopped, no more burning pixels found.");
                break;
            }
            n_rounds++;
            ixmin = ixmin2; ixmax = ixmax2; iymin = iymin2; iymax = iymax2;
            if (lg->should_log(spdlog::level::debug))
                lg->debug("Round {}, burned: {} ha, max-size: {} ha, rectangle: {}/{}-{}/{}", n_rounds, n_ha, max_ha, ixmin, iymin, ixmax, iymax);

        } // end while
    } // end if (fire at ignition point)

    lg->info("FireEvent. total burned (ha): {}, high severity (ha): {}, max-fire-size (ha): {}", n_ha, n_highseverity_ha, max_ha);
    SFireStat stat;
    stat.year = Model::instance()->year();
    stat.Id = ign.Id;
    stat.x = ign.x;
    stat.y = ign.y;
    stat.max_size = max_ha;
    stat.ha_burned = n_ha;
    stat.ha_high_severity = n_highseverity_ha;
    mStats.push_back(stat);
}


// examine a single cell and eventually burn.
bool FireModule::burnCell(int ix, int iy, int &rHighSeverity, int round)
{
    auto &grid = Model::instance()->landscape()->grid();
    auto &c = mGrid[Point(ix, iy)];
    auto &s = grid[Point(ix, iy)];
    if (s.isNull()) {
        c.spread = -1.f;
        if (round==1)
            lg->debug("Stopped at ignition: invalid cell!");
        return false;
    }

    // If a cell is already altered *during* this year (e.g. by a previous)
    // fire, then the state used here is still the old (i.e. unburned) state.
    // Can happen that a cells burns twice a year
    // a workaround could be: if cell->isUpdated() then then cell is already changed; in
    // this case, next years' state could be checked instead
    // (not sure if this plays nicely with other combinations of modules / management, ...)


    // burn probability
    double pBurn = s.state()->value(miBurnProbability);
    if (round>3) {
        pBurn *= (1. - mExtinguishProb);
    }
    if (pBurn == 0. || pBurn < drandom()) {
        if (round==1)
            lg->debug("Stopped at ignition: State: {} burn-prob: {}", s.state()->asString(), pBurn);
        c.spread = -1.f;
        return false;
    }

    bool high_severity = drandom() < s.state()->value(miHighSeverity);
    // effect of fire: a transition to another state
    state_t new_state = mFireMatrix.transition(s.stateId(), high_severity ? 1 : 0);
    s.setNewState(new_state);

    // test for landcover change
    if (lg->should_log(spdlog::level::trace))
        if ( s.state()->type() != Model::instance()->states()->stateById(new_state).type() )
            lg->trace("Landcover type change: from state '{}' to '{}'", s.stateId(), new_state);

    c.last_burn = static_cast<short int>(Model::instance()->year());
    c.n_fire++;
    if (high_severity) {
        c.n_high_severity++;
        rHighSeverity++;
    }
    c.spread = 1.f; // this cell spreads
    return true;
}

/// calculate effect of slope on fire spread
/// for upslope following Keene and Albini 1976
///  It was designed by RKeane (2/2/99) (calc.c)
/// the downslope function is "not based on empirical data" (Keane in calc.c)
/// return is the metric distance to spread (and not number of pixels)
double FireModule::calcSlopeFactor(const double slope) const
{
    double slopespread;       /* Slope spread rate in pixels / timestep   */
    static double firebgc_cellsize = 30.; /* cellsize for which this functions were originally designed */

    if (slope < 0.) {
        // downslope effect
        slopespread = 1.0 - ( 20.0 * slope * slope );

    } else {
        // upslope effect
        static double alpha = 4.0; /* Maximum number of pixels to spread      */
        static double beta  = 3.5; /* Scaling coeff for inflection point      */
        static double gamma = 10.0;/* Scaling coeff for graph steepness       */
        static double zeta  = 0.0; /* Scaling coeff for y intercept           */

        slopespread = zeta + ( alpha / ( 1.0 + ( beta * exp( -gamma * slope ) ) ) );
    }


    return( slopespread ) * firebgc_cellsize;

}

/// calculate the effect of wind on the spread.
/// function designed by R. Keane, 2/2/99
/// @param direction direction (in degrees) of spread (0=north, 90=east, ...)
/// @return spread (in meters)
double FireModule::calcWindFactor(const SIgnition &fire_event, const double direction) const
{
    const double firebgc_cellsize = 30.; /* cellsize for which this functions were originally designed */
    double windspread;         /* Wind spread rate in pixels / timestep   */
    double coeff;              /* Coefficient that reflects wind direction*/
    double lwr;                /* Length to width ratio                   */
    const double alpha = 0.6; /* Wind spread power coeffieicnt           */
    const double MPStoMPH = 1. / 0.44704;

    /* .... If zero wind speed return 1.0 for the factor .... */
    if ( fire_event.wind_speed <= 0.5 )
        return ( 1.0 ) * firebgc_cellsize; // not 0????

    /* .... Change degrees to radians .... */
    coeff = fabs( direction - fire_event.wind_direction ) * M_PI/180.;

    /* .... If spread direction equal zero, then spread direction = wind direct */
    if ( direction <= 0.01 )
        coeff = 0.0;

    /* .... Compute the length:width ratio from Andrews (1986) .....  */

    lwr = 1.0 + ( 0.125 * fire_event.wind_speed * MPStoMPH );

    /* .... Scale the difference between direction between 0 and 1.0 .....  */
    coeff = ( cos( coeff ) + 1.0 ) / 2.0;

    /* .... Scale the function based on windspeed between 1 and 10...  */
    windspread = pow( coeff, pow( (fire_event.wind_speed * MPStoMPH ), alpha ) ) * lwr;

    return( windspread ) * firebgc_cellsize;

}


/** calculates probability of spread from one pixel to one neighbor.
    In this functions the effect of the terrain, the wind and others are used to estimate a probability.
    @param fire_data reference to the variables valid for the current resource unit
    @param height elevation (m) of the origin point
    @param pixel_from pointer to the origin point in the fire grid
    @param pixel_to pointer to the target pixel
    @param direction codes the direction from the origin point (1..8, N, E, S, W, NE, SE, SW, NW)
  */
void FireModule::calculateSpreadProbability(const SIgnition &fire_event,  const Point &point, const float origin_elevation,  const int direction)
{

    if (!mGrid.isIndexValid(point) || Model::instance()->landscape()->grid()[point].isNull())
        return;

    auto & fire_cell = mGrid[point];

    if (fire_cell.spread<0.f || fire_cell.spread>=1.f)
        return;

    const double directions[8]= {0., 90., 180., 270., 45., 135., 225., 315. };
    double spread_metric; // distance that fire supposedly spreads

    // calculate the slope from the curent point (pixel_from) to the spreading cell (pixel_to)
    float h_to = Model::instance()->landscape()->grid()[point].elevation();
    if (h_to==0.f) {
        lg->debug("Invalid elevation (value = 0) at point '{}m/{}m'", mGrid.cellCenterPoint(point).x(), mGrid.cellCenterPoint(point).y());
        return;
    }
    double pixel_size = 100.;
    // if we spread diagonal, the distance is longer:
    if (direction>4)
        pixel_size *= 1.41421356;

    double slope = (h_to - origin_elevation) / pixel_size;

    double r_wind, r_slope; // metric distance for spread
    r_slope = calcSlopeFactor( slope ); // slope factor (upslope / downslope)

    r_wind = calcWindFactor(fire_event, directions[direction-1]); // metric distance from wind

    spread_metric = r_slope + r_wind;

    double spread_pixels = spread_metric / pixel_size;
    if (spread_pixels<=0.)
        return;

    // calculate the probability: this is the chance
    double p_spread = pow(mSpreadToDistProb, 1. / spread_pixels);
    // apply the r_land factor that accounts for different land types
    //p_spread *= fire_data.mRefLand;
    // add probabilites
    //*pixel_to = static_cast<float>(1. - (1. - *pixel_to)*(1. - p_spread));
    fire_cell.spread = static_cast<float>(1. - (1. - fire_cell.spread)*(1. - p_spread));

}

