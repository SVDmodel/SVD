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
#ifndef RANDOMGEN_H
#define RANDOMGEN_H

#include <random>


class RandomGenerator {
  public:
    RandomGenerator() { dbl_dist = std::uniform_real_distribution<double>(0., 1.);  setRandomSeed(); }
    // this is the more "correct" way of doing it, but it is three times slower than the other way
    //static double rand() { return dbl_dist(generator); }
    // the faster way
    static double rand() { return generator() / static_cast<double>(generator.max()); }
    static double rand(double range) { return rand()*range; }
    static int randInt(int range) { int r = static_cast<int>( generator() % static_cast<unsigned long long>(range) ); return r; }
    // random seed....
    static void setRandomSeed();
private:
    static std::uniform_real_distribution<double> dbl_dist;
    static std::mt19937_64 generator;
};

/// ******************************************
/// Global access functions for random numbers
/// ******************************************


/// nrandom returns a random number from [p1, p2) -> p2 is not a possible result!
inline double nrandom(const double& p1, const double& p2)
{
    return p1 + RandomGenerator::rand(p2-p1);
    //return p1 + (p2-p1)*(rand()/double(RAND_MAX));
}
/// returns a random number in [0,1) (i.e.="1" is NOT a possible result!)
inline double drandom()
{
    return RandomGenerator::rand();
    //return rand()/double(RAND_MAX);
}
/// return a random number from "from" to "to" (excluding 'to'.), i.e. irandom(3,6) results in 3, 4 or 5.
inline int irandom(int from, int to)
{
    return from + RandomGenerator::randInt(to-from);
    //return from +  rand()%(to-from);
}





#endif // RANDOMGEN_H
