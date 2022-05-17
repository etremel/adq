#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <vector>

#include "adq/core/InternalTypes.hpp"
#include "adq/util/Overlay.hpp"
#include "adq/messaging/ValueContribution.hpp"
#include "adq/messaging/ValueTuple.hpp"
#include "adq/util/TimerManager.hpp"
#include "adq/util/PointerUtil.hpp"
#include "CrusaderAgreementState.hpp"

namespace adq {
//Forward declaration
class TreeAggregationState;

enum class ProtocolPhase { IDLE,
                           SETUP,
                           SHUFFLE,
                           AGREEMENT,
                           AGGREGATE };

template <typename RecordType>
class ProtocolState {
private:
    std::shared_ptr<spdlog::logger> logger;
    ProtocolPhase protocol_phase;

    /** The ID of the meter that this ProtocolState tracks state for */
    int meter_id;
    /** The effective number of meters in the network, including virtual meters */
    int num_meters;
    /** Log (base 2) of num_meters */
    int log2n;
    /** This is a constant, but it must be set by the implementing subclass based on which algorithm it is using. */
    const int num_aggregation_groups;
    int overlay_round;
    bool is_last_round;
    /** The set of meters (by ID) that have definitely failed this round.
     * Meters are added to this set when this meter fails to establish a TCP
     * connection to them, and we don't bother waiting for a message from a
     * meter that has failed. */
    std::set<int> failed_meter_ids;

    /** Handle for the timer registered to timeout the round. */
    util::timer_id_t round_timeout_timer;
    bool ping_response_from_predecessor;
    template<typename T> using ptr_list = std::list<std::shared_ptr<T>>;
    ptr_list<messaging::OverlayTransportMessage> future_overlay_messages;
    ptr_list<messaging::AggregationMessage> future_aggregation_messages;
    ptr_list<messaging::OverlayMessage> waiting_messages;
    ptr_list<messaging::OverlayMessage> outgoing_messages;

    std::shared_ptr<messaging::ValueTuple<RecordType>> my_contribution;
    /** This automatically rejects duplicate proxy contributions; it must
     * be an unordered_set because there's no sensible way to "sort" contributions.
     * Since the set of proxies is part of ValueContribution's value equality
     * (by way of ValueTuple), two meters are allowed to contribute the same
     * measurement (they should have distinct proxy sets). */
    util::unordered_ptr_set<messaging::ValueContribution<RecordType>> proxy_values;
    std::unique_ptr<TreeAggregationState> aggregation_phase_state;

    std::unique_ptr<util::TimerManager> timers;
    NetworkManager<RecordType> network;
    CryptoLibrary crypto;

    /* --- Specific to BFT agreement --- */
    std::unique_ptr<CrusaderAgreementState> agreement_phase_state;
    int agreement_start_round;
    util::unordered_ptr_set<messaging::ValueContribution<RecordType>> accepted_proxy_values;
    /* ------ */

    void encrypted_multicast_to_proxies(const std::shared_ptr<messaging::ValueContribution<RecordType>>& contribution);
    void start_aggregate_phase();


public:
    ProtocolState(int num_clients, int local_client_id)
        : logger(spdlog::get("global_logger")),
          protocol_phase(ProtocolPhase::IDLE),
          agreement_start_round(0) {}

    void start_query(const std::shared_ptr<messaging::QueryRequest>& query_request, const RecordType& contributed_data);
    void handle_overlay_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
    void handle_aggregation_message(const std::shared_ptr<messaging::AggregationMessage>& message);
    void handle_ping_message(const std::shared_ptr<messaging::PingMessage>& message);
    void buffer_future_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
    void buffer_future_message(const std::shared_ptr<messaging::AggregationMessage>& message);

    int get_num_aggregation_groups() const { return num_aggregation_groups; }
    int get_current_query_num() const { return my_contribution ? my_contribution->query_num : -1; }
    int get_current_overlay_round() const { return overlay_round; }
};
}  // namespace adq

#include "detail/ProtocolState_impl.hpp"
