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
#include "randomgen.h"

#include <random>
#include <chrono>

std::uniform_real_distribution<double> RandomGenerator::dbl_dist = std::uniform_real_distribution<double>(0., 1.);
std::mt19937_64 RandomGenerator::generator;

// uniform_real_distribution: 1'000'000'000 random numbers: 27 secs (release mode)


void RandomGenerator::setRandomSeed()
{
    size_t seed = static_cast<size_t>(std::chrono::system_clock::now().time_since_epoch().count());
    generator.seed(seed);

}
