#pragma once

#include <adq/core/DataSource.hpp>
#include <adq/core/InternalTypes.hpp>
#include <adq/core/QueryFunctions.hpp>
#include <adq/util/FixedPoint.hpp>

#include <iostream>
#include <list>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace smart_meters {
/**
 * A simple data object holding the characteristics of a simulated electricity-using
 * device. Collections of devices are used to generate simulated electrical
 * consumption for a SimSmartMeter.
 */
struct Device {
    /** The device's name, as used in the configuration files; must uniquely identify the device */
    std::string name;
    /** in Watts; array size is number of possible unique cycles */
    std::vector<adq::FixedPoint_t> load_per_cycle;
    /**in minutes; should align with loadPerCycle array */
    std::vector<int> time_per_cycle;
    /** in Watts */
    adq::FixedPoint_t standby_load;
    double weekday_frequency;  // mean daily starting frequency
    double weekend_frequency;
    /** hour-by-hour usage probability for weekdays */
    std::vector<double> weekday_hourly_probability;
    /** hour-by-hour usage probability for weekends */
    std::vector<double> weekend_hourly_probability;
    /** if the customer will turn off this device when they can't afford the energy prices */
    bool disable_to_save_money;

    Device() : name(""), standby_load(0.0), weekday_frequency(0), weekend_frequency(0), disable_to_save_money(false){};
};

inline std::ostream& operator<<(std::ostream& stream, const Device& device) {
    return stream << "{" << device.name
                  << " | Load per cycle: " << device.load_per_cycle
                  << "| Time per cycle: " << device.time_per_cycle
                  << "| Standby load: " << device.standby_load
                  << "| Weekday frequency: " << device.weekday_frequency
                  << "| Weekend frequency: " << device.weekend_frequency
                  << "}";
}

struct DeviceState {
    /** time the device was actually started today */
    int start_time;
    /** time the device is scheduled to start, if it is shiftable and being delayed */
    int scheduled_start_time;
    bool is_on;
    int current_cycle_num;
    /** Number of minutes into the current cycle, if a timestep occurred in the middle of a cycle */
    int time_in_current_cycle;
};

/**
 * Represents the income level of a simulated home with a smart meter.
 * This affects how many devices it has, among other things.
 */
enum class IncomeLevel { POOR,
                         AVERAGE,
                         RICH };

/**
 * Represents a simulated meter, which generates simulated electrical consumption
 * measurements based on a list of devices in the "home" it is attached to. This
 * implements the adq::DataSource interface by registering some of its member
 * functions as Select, Filter, and Aggregate functions in the constructor.
 */
class SimSmartMeter : public adq::DataSource<std::vector<adq::FixedPoint_t>> {
public:
    /** A type alias for the type this class supplies as the RecordType parameter for adq::DataSource */
    using DataRecordType = std::vector<adq::FixedPoint_t>;

private:
    std::vector<std::pair<Device, DeviceState>> shiftable_devices;
    std::vector<std::pair<Device, DeviceState>> nonshiftable_devices;
    IncomeLevel income_level;
    int current_timestep;
    std::vector<adq::FixedPoint_t> consumption;
    std::vector<adq::FixedPoint_t> shiftable_consumption;
    std::mt19937 random_engine;
    /** Thread that advances the meter's simulated time at regular intervals of real time */
    std::thread simulation_thread;

    /**
     * Simulates a single device for a single timestep of time, and returns the
     * amount of power in watt-hours that device used during the timestep. The
     * device's state will be updated to reflect the cycle it's in at the end
     * of the timestep, if it's a device with multiple cycles. The device is
     * assumed to be on when this method is called (device.isOn == true),
     * since it doesn't make sense to call this method on a device that is off.
     *
     * @param device A device object
     * @param device_state The device's associated DeviceState object
     * @return The number of watt-hours of power consumed by this device in
     * the simulated timestep
     */
    adq::FixedPoint_t run_device(Device& device, DeviceState& device_state);
    adq::FixedPoint_t simulate_nonshiftables(int time);
    adq::FixedPoint_t simulate_shiftables(int time);
    adq::FixedPoint_t measure(const std::vector<adq::FixedPoint_t>& data, const int window_minutes) const;

public:
    enum SelectFunctions : adq::Opcode {
        MEASURE_CONSUMPTION = 0,
        MEASURE_SHIFTABLE_CONSUMPTION = 1,
        MEASURE_DAILY_CONSUMPTION = 2,
        SIMULATE_PROJECTED_USAGE = 3
    };
    enum FilterFunctions : adq::Opcode {
        NO_FILTER = 0
    };
    enum AggregateFunctions : adq::Opcode {
        SUM_VECTORS = 0
    };
    SimSmartMeter(const IncomeLevel& income_level, std::list<Device>& owned_devices);
    ~SimSmartMeter();
    adq::FixedPoint_t measure_consumption(const int window_minutes) const;
    adq::FixedPoint_t measure_shiftable_consumption(const int window_minutes) const;
    adq::FixedPoint_t measure_daily_consumption() const;
    std::vector<adq::FixedPoint_t> simulate_projected_usage(const int time_window);
    /** Simulates one timestep of energy usage and updates the internal vectors. */
    void simulate_usage_timestep();
    /**
     * Starts the simulation thread, which will advance the meter's simulated time
     * based on the configured value of TIME_PER_TIMESTEP in SimParameters.hpp
     */
    void run_simulation();
};

}  // namespace smart_meters