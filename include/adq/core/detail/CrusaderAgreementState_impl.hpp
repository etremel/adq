#pragma once

#include "../CrusaderAgreementState.hpp"

#include "adq/core/CryptoLibrary.hpp"
#include "adq/messaging/AgreementValue.hpp"
#include "adq/messaging/OverlayMessage.hpp"
#include "adq/messaging/PathOverlayMessage.hpp"
#include "adq/messaging/SignedValue.hpp"
#include "adq/messaging/ValueContribution.hpp"
#include "adq/util/PathFinder.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace adq {

template <typename RecordType>
std::vector<std::shared_ptr<messaging::OverlayMessage<RecordType>>> CrusaderAgreementState<RecordType>::finish_phase_1(int current_round) {
    std::vector<std::shared_ptr<messaging::OverlayMessage<RecordType>>> accept_messages;
    for(const auto& signed_value_entry : signed_proxy_values) {
        if(signed_value_entry.second.signatures.size() < (unsigned)log2n + 1) {
            // Reject values without enough signatures
            continue;
        }
        // Sign the accepted value
        auto signed_accepted_value = std::make_shared<messaging::AgreementValue<RecordType>>(signed_value_entry.second, node_id);
        crypto_library.rsa_sign(signed_value_entry.second, signed_accepted_value->accepter_signature);
        // Multicast it to the other proxies (besides this node)
        std::vector<int> other_proxies(signed_value_entry.first->value_tuple.proxies.size() - 1);
        std::remove_copy(signed_value_entry.first->value_tuple.proxies.begin(),
                         signed_value_entry.first->value_tuple.proxies.end(), other_proxies.begin(), node_id);
        auto proxy_paths = util::find_paths(node_id, other_proxies, num_nodes, current_round + 1);
        for(const auto& proxy_path : proxy_paths) {
            auto accept_message = std::make_shared<messaging::PathOverlayMessage<RecordType>>(
                query_num, proxy_path, signed_accepted_value);
            crypto_library.rsa_encrypt(*accept_message, proxy_path.back());
            accept_messages.emplace_back(std::move(accept_message));
        }
    }
    phase_1_finished = true;
    return accept_messages;
}

template <typename RecordType>
util::unordered_ptr_set<messaging::ValueContribution<RecordType>> CrusaderAgreementState<RecordType>::finish_phase_2() {
    util::unordered_ptr_set<messaging::ValueContribution<RecordType>> accepted_proxy_values;
    for(const auto& signed_value_entry : signed_proxy_values) {
        // Accept the value if it has enough distinct signatures
        if(signed_value_entry.second.signatures.size() >= (unsigned)log2n + 1) {
            accepted_proxy_values.emplace(signed_value_entry.second.value);
        } else {
            // Log warning
        }
    }
    return accepted_proxy_values;
}

template <typename RecordType>
void CrusaderAgreementState<RecordType>::handle_message(const messaging::OverlayMessage<RecordType>& message) {
    if(auto signed_value = std::dynamic_pointer_cast<messaging::SignedValue<RecordType>>(message.enclosed_body)) {
        handle_phase_1_message(*signed_value);
    } else if(auto agreement_value = std::dynamic_pointer_cast<messaging::AgreementValue<RecordType>>(message.enclosed_body)) {
        handle_phase_2_message(*agreement_value);
    }
}

template <typename RecordType>
void CrusaderAgreementState<RecordType>::handle_phase_1_message(const messaging::SignedValue<RecordType>& signed_value) {
    if(signed_value.signatures.empty()) {
        // Rejected a value without a signature!
        return;
    }
    // The message's signature map should have only one entry in it
    auto signature_pair = *signed_value.signatures.begin();
    if(!crypto_library.rsa_verify(*signed_value.value, signature_pair.second, signature_pair.first)) {
        // Rejected an invalid signature!
        return;
    }
    // If this is the first signature received for the value, put it in the map.
    // Otherwise, add the signature to the list of signatures already in the map.
    auto signed_proxy_values_find = signed_proxy_values.find(signed_value.value);
    if(signed_proxy_values_find == signed_proxy_values.end()) {
        signed_proxy_values[signed_value.value] = signed_value;
    } else {
        signed_proxy_values_find->second.signatures.insert(signature_pair);
    }
}

template <typename RecordType>
void CrusaderAgreementState<RecordType>::handle_phase_2_message(messaging::AgreementValue<RecordType>& agreement_value) {
    // Verify the sender's signature on the message
    if(!crypto_library.rsa_verify(agreement_value.signed_value, agreement_value.accepter_signature, agreement_value.accepter_id)) {
        // Rejected a message for an invalid signature!
        return;
    }
    // Validate each signature in the package, and remove invalid ones
    int valid_signatures = 0;
    for(auto signatures_iter = agreement_value.signed_value.signatures.begin();
        signatures_iter != agreement_value.signed_value.signatures.end();) {
        // The sender's signature doesn't count towards receiving at least t signatures
        if(signatures_iter->first == agreement_value.accepter_id) {
            ++signatures_iter;
            continue;
        }
        if(crypto_library.rsa_verify(*agreement_value.signed_value.value, signatures_iter->second, signatures_iter->first)) {
            ++valid_signatures;
            ++signatures_iter;
        } else {
            signatures_iter = agreement_value.signed_value.signatures.erase(signatures_iter);
        }
    }

    if(valid_signatures >= log2n) {
        auto signed_proxy_values_find = signed_proxy_values.find(agreement_value.signed_value.value);
        if(signed_proxy_values_find == signed_proxy_values.end()) {
            signed_proxy_values[agreement_value.signed_value.value] = agreement_value.signed_value;
        } else {
            signed_proxy_values_find->second.signatures.insert(agreement_value.signed_value.signatures.begin(),
                                                               agreement_value.signed_value.signatures.end());
        }
    } else {
        // Log warning
    }
}

}  // namespace adq