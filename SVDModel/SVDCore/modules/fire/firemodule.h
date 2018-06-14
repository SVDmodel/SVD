#ifndef FIREMODULE_H
#define FIREMODULE_H

#include <map>

#include "transitionmatrix.h"
#include "states.h"
#include "grid.h"
#include "spdlog/spdlog.h"

struct SFireCell {
    SFireCell() : spread(0), n_fire(0), n_high_severity(0), last_burn(0) {}
    short int spread; ///< spread flag during current fire event
    short int n_fire; ///< counter how often cell burned
    short int n_high_severity; ///< high severity counter
    short int last_burn; ///< year when the cell burned the last time
};

class FireModule
{
public:
    FireModule();
    void setup();

    int run();

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
    int miBurnProbability;
    int miHighSeverity;
};

#endif // FIREMODULE_H
