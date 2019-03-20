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
#ifndef FIREMODULE_H
#define FIREMODULE_H

#include <map>

#include "modules/module.h"

#include "transitionmatrix.h"
#include "states.h"
#include "grid.h"
#include "spdlog/spdlog.h"

class FireOut; // forward

struct SFireCell {
    SFireCell() : spread(0), n_fire(0), n_high_severity(0), last_burn(0) {}
    short int spread; ///< spread flag during current fire event
    short int n_fire; ///< counter how often cell burned
    short int n_high_severity; ///< high severity counter
    short int last_burn; ///< year when the cell burned the last time
};

struct SFireStat {
    int year; ///< year of the fire
    double x, y; ///<  ignition point (metric coords)
    int max_size; ///< maximum fire size (ha)
    int ha_burned; ///< # of ha. burned
    int ha_high_severity; ///< # of ha burned with high severity

};

class FireModule : public Module
{
public:
    FireModule(std::string module_name);
    void setup();
    std::vector<std::pair<std::string, std::string> > moduleVariableNames() const;
    virtual double moduleVariable(const Cell *cell, size_t variableIndex) const;

    void run();

    // getters
    const Grid<SFireCell> &fireGrid() { return mGrid; }

private:

    // logging
    std::shared_ptr<spdlog::logger> lg;


    // store for ignitions
    struct SIgnition {
        SIgnition(int ayear, double ax, double ay, double amax): year(ayear), x(ax), y(ay), max_size(amax) {}
       int year;
       double x, y; // cooridnates (meter)
       double max_size; // maximum fire size in m2
    };
    std::multimap< int, SIgnition > mIgnitions;

    Grid<SFireCell> mGrid;

    void fireSpread(const SIgnition &ign);
    bool burnCell(int ix, int iy, int &rHighSeverity, int round);


    // store for transition probabilites for burned cells
    TransitionMatrix mFireMatrix;

    double mExtinguishProb;

    // index of variables
    size_t miBurnProbability;
    size_t miHighSeverity;

    // fire statistics
    std::vector< SFireStat > mStats;

    friend class FireOut;
};

#endif // FIREMODULE_H
