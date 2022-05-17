
#include <adq/core/DataSource.hpp>
#include <adq/core/InternalTypes.hpp>
#include <adq/core/QueryFunctions.hpp>
#include <adq/util/FixedPoint.hpp>

#include <random>
#include <vector>

class SimSmartMeter : public adq::DataSource<std::vector<adq::FixedPoint_t>> {
private:
    std::vector<adq::FixedPoint_t> consumption;
    std::vector<adq::FixedPoint_t> shiftable_consumption;
    std::mt19937 random_engine;

public:
    enum SelectFunctions : adq::Opcode {
        MEASURE_CONSUMPTION = 0,
        MEASURE_SHIFTABLE_CONSUMPTION = 1,
        MEASURE_DAILY_CONSUMPTION = 2,
        SIMULATE_PROJECTED_USAGE = 3
    };
    SimSmartMeter();
    adq::FixedPoint_t measure_consumption(const int window_minutes) const;
    adq::FixedPoint_t measure_shiftable_consumption(const int window_minutes) const;
    adq::FixedPoint_t measure_daily_consumption() const;
    std::vector<adq::FixedPoint_t> simulate_projected_usage(const int time_window);
    /** Simulates one timestep of energy usage and updates the internal vectors. */
    void simulate_usage_timestep();
};