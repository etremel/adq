#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <vector>

namespace adq {

enum class ProtcolPhase { IDLE,
                          SETUP,
                          SHUFFLE,
                          AGREEMENT,
                          AGGREGATE };

class ProtocolState {
private:
    std::shared_ptr<spdlog::logger> logger;
    ProtocolPhase protocol_phase;
    int agreement_start_round;

public:
    ProtocolState(int num_clients, int local_client_id)
        : logger(spdlog::get("global_logger")),
          protocol_phase(ProtocolPhase::IDLE),
          agreement_start_round(0) {}
}
}  // namespace adq
