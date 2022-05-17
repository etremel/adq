#include "SimSmartMeter.hpp"

SimSmartMeter::SimSmartMeter() {
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