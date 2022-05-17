#include "../ProtocolState.hpp"

namespace adq {

template<typename RecordType>
void ProtocolState<RecordType>::start_query(const std::shared_ptr<messaging::QueryRequest>& query_request, const RecordType& contributed_data) {
    overlay_round = -1;
    is_last_round = false;
    ping_response_from_predecessor = false;
    timers->cancel_timer(round_timeout_timer);
    proxy_values.clear();
    failed_meter_ids.clear();
    aggregation_phase_state = std::make_unique<TreeAggregationState>(meter_id, num_aggregation_groups, num_meters,
            network, query_request);
    std::vector<int> proxies = util::pick_proxies(meter_id, num_aggregation_groups, num_meters);
    logger->trace("Meter {} chose these proxies: {}", meter_id, proxies);
    my_contribution = std::make_shared<messaging::ValueTuple>(query_request->query_number, contributed_data, proxies);

    protocol_phase = ProtocolPhase::SETUP;
    accepted_proxy_values.clear();
    //Encrypt my ValueTuple and send it to the utility to be signed
    auto encrypted_contribution = crypto.rsa_encrypt(my_contribution, meter_id);
    network.send(std::make_shared<messaging::SignatureRequest>(meter_id, encrypted_contribution));
}

}