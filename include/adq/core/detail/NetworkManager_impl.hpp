#include "../NetworkManager.hpp"

#include "adq/core/QueryClient.hpp"
#include "adq/messaging/AggregationMessage.hpp"
#include "adq/messaging/MessageType.hpp"
#include "adq/messaging/OverlayTransportMessage.hpp"
#include "adq/messaging/PingMessage.hpp"
#include "adq/messaging/QueryRequest.hpp"
#include "adq/messaging/SignatureResponse.hpp"

#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <asio.hpp>

namespace adq {

template <typename RecordType>
NetworkManager<RecordType>::NetworkManager(QueryClient<RecordType>& owning_client,
                                           const asio::ip::tcp::endpoint& my_tcp_address,
                                           const std::map<int, asio::ip::tcp::endpoint>& client_id_to_ip_map)
    : logger(spdlog::get("global_logger")),
      query_client(owning_client),
      id_to_ip_map(client_id_to_ip_map),
      connection_listener(network_io_context) {
    // Initialize the receive buffers with empty arrays, and initialize the reverse IP-to-ID map
    for(const auto& id_ip_pair : client_id_to_ip_map) {
        receive_buffers.emplace(id_ip_pair.first, std::array<uint8_t, sizeof(std::size_t)>{});
        ip_to_id_map.emplace(id_ip_pair.second, id_ip_pair.first);
    }
    do_accept();
}

template <typename RecordType>
void NetworkManager<RecordType>::do_accept() {
    connection_listener.async_accept(network_io_context,
                                     [this](const asio::error_code& error, asio::ip::tcp::socket peer) {
                                         handle_accept(error, std::move(peer));
                                     });
}

template <typename RecordType>
void NetworkManager<RecordType>::handle_accept(const asio::error_code& error, asio::ip::tcp::socket new_socket) {
    // Put the new socket in the map
    asio::ip::tcp::endpoint client_ip = new_socket.remote_endpoint();
    int client_id = ip_to_id_map.at(client_ip);
    sockets_by_id.emplace(client_id, std::move(new_socket));
    // Enqueue an async read for the first 4 bytes (size of the message)
    do_read(client_id);
    // Enqueue another accept operation for the connection listener so it keeps listening
    do_accept();
}

template <typename RecordType>
void NetworkManager<RecordType>::do_read(int client_id) {
    asio::async_read(sockets_by_id.at(client_id),
                     asio::buffer(receive_buffers.at(client_id)),
                     [this, client_id](const asio::error_code& error, std::size_t bytes_transferred) {
                         handle_read(client_id, error, bytes_transferred);
                     });
}

template <typename RecordType>
void NetworkManager<RecordType>::handle_read(int client_id, const asio::error_code& error, std::size_t bytes_read) {
    // Assume for now there was no error. Read the size of the message from the buffer.
    std::size_t message_size = reinterpret_cast<std::size_t*>(receive_buffers.at(client_id).data())[0];
    // Create a buffer of the correct size
    std::vector<uint8_t> message_buffer(message_size);
    // Do a synchronous read for the rest of the message
    asio::read(sockets_by_id.at(client_id), asio::buffer(message_buffer));
    // Send it to the application-level handler
    receive_message(message_buffer);
}

template <typename RecordType>
void NetworkManager<RecordType>::receive_message(const std::vector<uint8_t>& message_bytes) {
    using namespace messaging;
    // First, read the number of messages in the list
    std::size_t num_messages = ((std::size_t*)message_bytes.data())[0];
    const uint8_t* buffer = message_bytes.data() + sizeof(num_messages);
    // Deserialize that number of messages, moving the buffer pointer each time one is deserialized
    for(auto i = 0u; i < num_messages; ++i) {
        /* This is the exact same logic used in Message::from_bytes. We could just do
         * auto message = mutils::from_bytes<messaging::Message>(nullptr, message_bytes.data());
         * but then we would have to use dynamic_pointer_cast to figure out which subclass
         * was deserialized and call the right query_client.handle_message() overload.
         */
        MessageType message_type = ((MessageType*)(buffer))[0];
        // Deserialize the correct message subclass based on the type, and call the correct handler
        switch(message_type) {
            case OverlayTransportMessage::type: {
                std::shared_ptr<OverlayTransportMessage> message(mutils::from_bytes<OverlayTransportMessage>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                query_client.handle_message(message);
                break;
            }
            case PingMessage::type: {
                std::shared_ptr<PingMessage> message(mutils::from_bytes<PingMessage>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                query_client.handle_message(message);
                break;
            }
            case AggregationMessage::type: {
                std::shared_ptr<AggregationMessage> message(mutils::from_bytes<AggregationMessage>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                query_client.handle_message(message);
                break;
            }
            case QueryRequest::type: {
                std::shared_ptr<QueryRequest> message(mutils::from_bytes<QueryRequest>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                std::cout << "Received a QueryRequest: " << *message << std::endl;
                query_client.handle_message(message);
                break;
            }
            case SignatureResponse::type: {
                std::shared_ptr<SignatureResponse> message(mutils::from_bytes<SignatureResponse>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                query_client.handle_message(message);
                break;
            }
            default:
                logger->warn("Client {} dropped a message it didn't know how to handle.", query_client.my_id);
                break;
        }
    }
}

}  // namespace adq