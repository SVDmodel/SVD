#include "randomgen.h"

#include <random>
#include <chrono>

std::uniform_real_distribution<double> RandomGenerator::dbl_dist = std::uniform_real_distribution<double>(0., 1.);
std::mt19937_64 RandomGenerator::generator;

// uniform_real_distribution: 1'000'000'000 random numbers: 27 secs (release mode)


void RandomGenerator::setRandomSeed()
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    generator.seed(seed);

}
