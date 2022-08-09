#pragma once

#include "adq/messaging/SignedValue.hpp"
#include "adq/messaging/ValueContribution.hpp"
#include "adq/util/PointerUtil.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>
#include <vector>

namespace adq {
class CryptoLibrary;
namespace messaging {
template<typename RecordType>
struct AgreementValue;
} /* namespace messaging */
}  // namespace adq

namespace adq {

/**
 * State machine for the Crusader Agreement (2-phase Byzantine Agreement) phase
 * of the BFT query protocol. Keeps track of intermediate state of the
 * protocol and handles transitions based on received messages.
 */
template <typename RecordType>
class CrusaderAgreementState {
private:
    const int node_id;
    const int num_nodes;
    const int log2n;
    const int query_num;
    bool phase_1_finished;
    CryptoLibrary& crypto_library;
    //I want my map keys to be ValueContributions, but I have to store them by
    //pointer because this map doesn't own them. This mess is the result.
    std::unordered_map<
        std::shared_ptr<messaging::ValueContribution<RecordType>>,
        messaging::SignedValue<RecordType>,
        util::ptr_hash<messaging::ValueContribution<RecordType>>,
        util::ptr_equal<messaging::ValueContribution<RecordType>>>
        signed_proxy_values;

public:
    CrusaderAgreementState(const int node_id, const int num_nodes, const int query_num, CryptoLibrary& crypto_library)
        : node_id(node_id),
          num_nodes(num_nodes),
          log2n((int)std::ceil(std::log2(num_nodes))),
          query_num(query_num),
          phase_1_finished(false),
          crypto_library(crypto_library) {}

    bool is_phase1_finished() { return phase_1_finished; }
    std::vector<std::shared_ptr<messaging::OverlayMessage>> finish_phase_1(int current_round);
    util::unordered_ptr_set<messaging::ValueContribution<RecordType>> finish_phase_2();
    void handle_message(const messaging::OverlayMessage& message);

private:
    void handle_phase_1_message(const std::shared_ptr<messaging::SignedValue<RecordType>>& signed_value);
    void handle_phase_2_message(const std::shared_ptr<messaging::AgreementValue<RecordType>>& agreement_value);
};

} /* namespace adq */
