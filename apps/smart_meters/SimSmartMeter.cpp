#include "SimSmartMeter.hpp"
#include "SimTimesteps.hpp"

#include <adq/util/FixedPoint.hpp>

#include <cstdint>
#include <cstring>
#include <list>
#include <random>
#include <string>
#include <vector>

namespace smart_meters {

SimSmartMeter::SimSmartMeter(const IncomeLevel& income_level, std::list<Device>& owned_devices)
    : income_level(income_level), current_timestep(-1) {
    for(auto& device : owned_devices) {
        // Currently, only air conditioners are shiftable (smart thermostats)
        if(device.name.find("conditioner") != std::string::npos) {
            shiftable_devices.emplace_back(std::move(device), DeviceState{});
        } else {
            nonshiftable_devices.emplace_back(std::move(device), DeviceState{});
        }
    }
    // Register each "measure" function as a data-selection function
    select_functions[MEASURE_CONSUMPTION] = [this](const uint8_t* const serialized_args) {
        // Deserialize argument
        int window_minutes;
        std::memcpy(&window_minutes, serialized_args, sizeof(window_minutes));
        // Call function
        adq::FixedPoint_t consumption = measure_consumption(window_minutes);
        return std::vector<adq::FixedPoint_t>{consumption};
    };
    select_functions[MEASURE_SHIFTABLE_CONSUMPTION] = [this](const uint8_t* const serialized_args) {
        // Deserialize argument
        int window_minutes;
        std::memcpy(&window_minutes, serialized_args, sizeof(window_minutes));
        // Call function
        adq::FixedPoint_t consumption = measure_shiftable_consumption(window_minutes);
        return std::vector<adq::FixedPoint_t>{consumption};
    };
    select_functions[MEASURE_DAILY_CONSUMPTION] = [this](const uint8_t* const serialized_args) {
        // This function has no argument
        adq::FixedPoint_t consumption = measure_daily_consumption();
        return std::vector<adq::FixedPoint_t>{consumption};
    };
    select_functions[SIMULATE_PROJECTED_USAGE] = [this](const uint8_t* const serialized_args) {
        // Deserialize argument
        int window_minutes;
        std::memcpy(&window_minutes, serialized_args, sizeof(window_minutes));
        // Call function
        return simulate_projected_usage(window_minutes);
    };

    // Filter function zero is "no filter"
    filter_functions[0] = [](const std::vector<adq::FixedPoint_t>& record, const uint8_t* const serialized_args) {
        return true;
    };
}

SimSmartMeter::~SimSmartMeter() {
    if(simulation_thread.joinable()) {
        simulation_thread.join();
    }
}

void SimSmartMeter::simulate_usage_timestep() {
    current_timestep++;

    consumption[current_timestep] = simulate_nonshiftables(current_timestep);
    shiftable_consumption[current_timestep] = simulate_shiftables(current_timestep);
    consumption[current_timestep] += shiftable_consumption[current_timestep];

    // If the next timestep will be after midnight of a new day,
    // reset actual start time to "not yet run" for devices that have finished running today
    if(hour(current_timestep + 1) % 24 == 0) {
        for(auto& device_pair : shiftable_devices) {
            DeviceState& deviceState = device_pair.second;
            if(deviceState.is_on == false && deviceState.start_time != -1) {
                deviceState.start_time = -1;
                deviceState.current_cycle_num = 0;
            }
        }
        for(auto& device_pair : nonshiftable_devices) {
            DeviceState& deviceState = device_pair.second;
            if(deviceState.is_on == false && deviceState.start_time != -1) {
                deviceState.start_time = -1;
                deviceState.current_cycle_num = 0;
            }
        }
    }
}

adq::FixedPoint_t SimSmartMeter::simulate_nonshiftables(int time) {
    adq::FixedPoint_t total_consumption;
    for(auto& device_pair : nonshiftable_devices) {
        double step_factor = USAGE_TIMESTEP_MIN / 60.0;
        double hourly_factor;
        double frequency_factor;
        if(day(time) % 7 == 5 || day(time % 7 == 6)) {
            hourly_factor = device_pair.first.weekend_hourly_probability[hour(time) % 24];
            frequency_factor = device_pair.first.weekend_frequency;
        } else {
            hourly_factor = device_pair.first.weekday_hourly_probability[hour(time) % 24];
            frequency_factor = device_pair.first.weekday_frequency;
        }
        // Probability of starting = step_factor * hourly_factor * frequency_factor
        if(std::bernoulli_distribution{step_factor * hourly_factor * frequency_factor}(random_engine)) {
            device_pair.second.is_on = true;
        }
        if(device_pair.second.is_on) {
            device_pair.second.start_time = time;
            total_consumption += run_device(device_pair.first, device_pair.second);
        }
        // Regardless of whether device turned on, add its standby usage
        total_consumption += device_pair.first.standby_load * adq::FixedPoint_t(step_factor);
    }
    return total_consumption;
}

adq::FixedPoint_t SimSmartMeter::simulate_shiftables(int time) {
    adq::FixedPoint_t total_consumption;
    for(auto& device_pair : shiftable_devices) {
        if(device_pair.second.scheduled_start_time > -1) {
            if(!device_pair.second.is_on && time >= device_pair.second.scheduled_start_time) {
                device_pair.second.is_on = true;
                device_pair.second.start_time = time;
            }
        } else {
            double step_factor = USAGE_TIMESTEP_MIN / 60.0;
            double hourly_factor;
            double frequency_factor;
            if(day(time) % 7 == 5 || day(time % 7 == 6)) {
                hourly_factor = device_pair.first.weekend_hourly_probability[hour(time) % 24];
                frequency_factor = device_pair.first.weekend_frequency;
            } else {
                hourly_factor = device_pair.first.weekday_hourly_probability[hour(time) % 24];
                frequency_factor = device_pair.first.weekday_frequency;
            }
            // Probability of starting = step_factor * hourly_factor * frequency_factor
            if(std::bernoulli_distribution{step_factor * hourly_factor * frequency_factor}(random_engine)) {
                device_pair.second.is_on = true;
            }
        }
        if(device_pair.second.is_on) {
            total_consumption += run_device(device_pair.first, device_pair.second);
        }
        // If the device was scheduled and has just completed its run, reset it to non-scheduled
        // Note that run_device sets is_on back to false if the device finished running during this timestep
        if(time >= device_pair.second.scheduled_start_time && !device_pair.second.is_on) {
            device_pair.second.scheduled_start_time = -1;
        }
        // Regardless of whether device turned on, add its standby usage
        total_consumption += device_pair.first.standby_load * adq::FixedPoint_t(USAGE_TIMESTEP_MIN / 60.0);
    }
    return total_consumption;
}

adq::FixedPoint_t SimSmartMeter::run_device(Device& device, DeviceState& device_state) {
    adq::FixedPoint_t power_consumed;  // in watt-hours
    int time_remaining_in_timestep = USAGE_TIMESTEP_MIN;
    // Simulate as many device cycles as will fit in one timestep
    while(time_remaining_in_timestep > 0 && device_state.current_cycle_num < (int)device.load_per_cycle.size()) {
        // The device may already be partway through the current cycle
        int current_cycle_time = device.time_per_cycle[device_state.current_cycle_num] - device_state.time_in_current_cycle;
        // Simulate a partial cycle if the current cycle has more time remaining than the timestep
        int minutes_simulated = std::min(current_cycle_time, time_remaining_in_timestep);
        power_consumed += device.load_per_cycle[device_state.current_cycle_num] * adq::FixedPoint_t(minutes_simulated / 60.0);
        device_state.time_in_current_cycle += minutes_simulated;
        time_remaining_in_timestep -= minutes_simulated;
        // If we completed simulating a cycle, advance to the next one and check if there's time remaining in the timestep
        if(device_state.time_in_current_cycle == device.time_per_cycle[device_state.current_cycle_num]) {
            device_state.current_cycle_num++;
            device_state.time_in_current_cycle = 0;
        }
    }
    // If the loop stopped because the device finished its last cycle, turn it off
    if(device_state.current_cycle_num == (int)device.load_per_cycle.size()) {
        device_state.is_on = false;
        device_state.current_cycle_num = 0;
    }
    return power_consumed;
}

std::vector<adq::FixedPoint_t> SimSmartMeter::simulate_projected_usage(const int time_window) {
    int window_whole_timesteps = time_window / USAGE_TIMESTEP_MIN;
    adq::FixedPoint_t window_last_fraction_timestep(time_window / (double) USAGE_TIMESTEP_MIN - window_whole_timesteps);
    std::vector<adq::FixedPoint_t> projected_usage(window_whole_timesteps + 1);
    //save states of devices, which will be modified by the "fake" simulation
    std::vector<std::pair<Device, DeviceState>> shiftable_backup(shiftable_devices);
    std::vector<std::pair<Device, DeviceState>> nonshiftable_backup(nonshiftable_devices);
    //simulate the next time_window minutes under the proposed function
    for(int sim_ts = current_timestep; sim_ts < current_timestep + window_whole_timesteps + 1; ++sim_ts) {
        projected_usage[sim_ts-current_timestep] = simulate_nonshiftables(sim_ts)
                + simulate_shiftables(sim_ts);
    }
    projected_usage[window_whole_timesteps] *= window_last_fraction_timestep;
    //restore saved states
    std::swap(shiftable_devices, shiftable_backup);
    std::swap(nonshiftable_devices, nonshiftable_backup);
    return projected_usage;
}

adq::FixedPoint_t SimSmartMeter::measure(const std::vector<adq::FixedPoint_t>& data, const int window_minutes) const {
    adq::FixedPoint_t window_consumption;
    int windowWholeTimesteps = window_minutes / USAGE_TIMESTEP_MIN;
    if(windowWholeTimesteps > current_timestep) {
        //Caller requested more timesteps than have been simulated, so just return what we have
        for(int i = 0; i <= current_timestep; ++i) {
            window_consumption += data[i];
        }
    }
    else {
        double windowLastFractionTimestep = window_minutes / (double) USAGE_TIMESTEP_MIN - windowWholeTimesteps;
        for(int offset = 0; offset < windowWholeTimesteps; offset++) {
            window_consumption += data[current_timestep-offset];
        }
        window_consumption += (data[current_timestep-windowWholeTimesteps] * adq::FixedPoint_t(windowLastFractionTimestep));
    }
    return window_consumption;
}

adq::FixedPoint_t SimSmartMeter::measure_consumption(const int window_minutes) const {
    return measure(consumption, window_minutes);
}

adq::FixedPoint_t SimSmartMeter::measure_shiftable_consumption(const int window_minutes) const {
    return measure(shiftable_consumption, window_minutes);
}

adq::FixedPoint_t SimSmartMeter::measure_daily_consumption() const {
    int day_start = day(current_timestep) * (1440 / USAGE_TIMESTEP_MIN);
    adq::FixedPoint_t daily_consumption;
    for(int time = day_start; time < current_timestep; ++time) {
        daily_consumption += consumption[time];
    }
    return daily_consumption;
}

void SimSmartMeter::run_simulation() {
    simulation_thread = std::thread([this]() {
        for(int timestep = 0; timestep < TOTAL_TIMESTEPS; ++timestep) {
            simulate_usage_timestep();
            std::cout << "Advanced simulated meter by " << USAGE_TIMESTEP_MIN << " minutes" << std::endl;
            std::this_thread::sleep_for(TIME_PER_TIMESTEP);
        }
    });
}

}  // namespace smart_meters