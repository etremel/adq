#pragma once

#include <chrono>

/**
 * @file SimParameters.hpp
 * Contains configuration parameters for the simulated smart meters.
 * In the future, this should be replaced with a configuration file that gets
 * loaded by a parser like GetPot.
 */

namespace smart_meters {

const int SIMULATION_DAYS = 1;
/** The number of minutes represented by one usage timestep. To prevent rounding errors,
 * this should be a number that divides 1440 (the number of minutes in a day) */
const int USAGE_TIMESTEP_MIN = 10;

constexpr int TOTAL_TIMESTEPS = 144;

const int PERCENT_POOR_HOMES = 25;
const int PERCENT_RICH_HOMES = 25;

const int NUM_QUERIES = 3;

const auto TIME_PER_TIMESTEP = std::chrono::seconds(10);

}