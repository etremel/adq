#pragma once

#include <string>

namespace smart_meters {

// String constants for the configuration properties that the smart-meter simulation
// expects to find in the config.ini file, in addition to the standard "system" properties

extern const std::string SECTION_SIMULATION;

extern const std::string SIMULATION_DAYS;
/** The number of minutes represented by one usage timestep. To prevent rounding errors,
 * this should be a number that divides 1440 (the number of minutes in a day) */
extern const std::string USAGE_TIMESTEP_MIN;
/** The total number of timesteps to simulate during one run of the program */
extern const std::string TOTAL_TIMESTEPS;

extern const std::string PERCENT_POOR_HOMES;

extern const std::string PERCENT_RICH_HOMES;

/** The number of queries the simulated utility server should issue in one run of the program */
extern const std::string NUM_QUERIES;
/** The wall-clock time in milliseconds that a simulated meter should wait between timesteps */
extern const std::string MS_PER_TIMESTEP;

}
