#include "../NetworkManager.hpp"

#include "adq/core/MessageConsumer.hpp"
#include "adq/messaging/AggregationMessage.hpp"
#include "adq/messaging/MessageType.hpp"
#include "adq/messaging/OverlayTransportMessage.hpp"
#include "adq/messaging/PingMessage.hpp"
#include "adq/messaging/QueryRequest.hpp"
#include "adq/messaging/SignatureRequest.hpp"
#include "adq/messaging/SignatureResponse.hpp"

#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <asio.hpp>
#include <cassert>

namespace adq {

template <typename RecordType>
NetworkManager<RecordType>::NetworkManager(MessageConsumer<RecordType>* owning_client,
                                           uint16_t service_port,
                                           const std::map<int, asio::ip::tcp::endpoint>& client_id_to_ip_map)
    : logger(spdlog::get("global_logger")),
      message_handler(owning_client),
      id_to_ip_map(client_id_to_ip_map),
      connection_listener(network_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::tcp::v4(), service_port)) {
    assert(message_handler != nullptr);
    // Initialize the receive buffers with empty arrays, and initialize the reverse IP-to-ID map
    for(const auto& id_ip_pair : client_id_to_ip_map) {
        length_buffers.emplace(id_ip_pair.first, std::array<uint8_t, sizeof(std::size_t)>{});
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
NetworkManager<RecordType>::~NetworkManager() {
    shutdown();
}

template <typename RecordType>
void NetworkManager<RecordType>::handle_accept(const asio::error_code& error, asio::ip::tcp::socket new_socket) {
    // Put the new socket in the map
    asio::ip::tcp::endpoint client_ip = new_socket.remote_endpoint();
    int client_id = ip_to_id_map.at(client_ip);
    sockets_by_id.emplace(client_id, std::move(new_socket));
    // Enqueue an async read for the first 4 bytes (size of the message)
    start_size_read(client_id);
    // Enqueue another accept operation for the connection listener so it keeps listening
    do_accept();
}

template <typename RecordType>
void NetworkManager<RecordType>::start_size_read(int client_id) {
    asio::async_read(sockets_by_id.at(client_id),
                     asio::buffer(length_buffers.at(client_id)),
                     [this, client_id](const asio::error_code& error, std::size_t bytes_transferred) {
                         handle_size_read(client_id, error, bytes_transferred);
                     });
}

template <typename RecordType>
void NetworkManager<RecordType>::handle_size_read(int client_id, const asio::error_code& error, std::size_t bytes_read) {
    if(!error) {
        // Read the size of the message from the buffer.
        std::size_t message_size = reinterpret_cast<std::size_t*>(length_buffers.at(client_id).data())[0];
        // Start an async read for the rest of the message
        start_body_read(client_id, message_size);
    } else if(error == asio::error::eof || error == asio::error::connection_aborted) {
        logger->debug("Client {} disconnected before sending message length", client_id);
    } else {
        logger->error("Unexpected error while reading message size from client {}: {}", client_id, error.message());
    }
}

template <typename RecordType>
void NetworkManager<RecordType>::start_body_read(int client_id, std::size_t body_size) {
    // Create or resize a buffer to the correct size
    incoming_message_buffers[client_id].resize(body_size);
    asio::async_read(sockets_by_id.at(client_id),
                     asio::buffer(incoming_message_buffers.at(client_id)),
                     [this, client_id](const asio::error_code& error, std::size_t bytes_read) {
                         handle_body_read(client_id, error, bytes_read);
                     });
}

template <typename RecordType>
void NetworkManager<RecordType>::handle_body_read(int client_id, const asio::error_code& error, std::size_t bytes_read) {
    if(!error) {
        logger->debug("Received a message of size {} from client {}", incoming_message_buffers[client_id].size(), client_id);
        // Send it to the application-level handler
        receive_message(incoming_message_buffers[client_id]);
    } else if(error == asio::error::eof || error == asio::error::connection_aborted) {
        logger->debug("Client {} disconnected before sending entire message", client_id);
        sockets_by_id.erase(client_id);
    } else {
        logger->error("Unexpected error while reading message body from client {}: {}", client_id, error.message());
        sockets_by_id.erase(client_id);
    }
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
         * was deserialized and call the right message_handler.handle_message() overload.
         */
        MessageType message_type = ((MessageType*)(buffer))[0];
        // Deserialize the correct message subclass based on the type, and call the correct handler
        switch(message_type) {
            case OverlayTransportMessage::type: {
                std::shared_ptr<OverlayTransportMessage> message(mutils::from_bytes<OverlayTransportMessage>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                message_handler->handle_message(message);
                break;
            }
            case PingMessage::type: {
                std::shared_ptr<PingMessage> message(mutils::from_bytes<PingMessage>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                message_handler->handle_message(message);
                break;
            }
            case MessageType::AGGREGATION: {
                std::shared_ptr<AggregationMessage<RecordType>> message(mutils::from_bytes<AggregationMessage<RecordType>>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                message_handler->handle_message(message);
                break;
            }
            case QueryRequest::type: {
                std::shared_ptr<QueryRequest> message(mutils::from_bytes<QueryRequest>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                std::cout << "Received a QueryRequest: " << *message << std::endl;
                message_handler->handle_message(message);
                break;
            }
            case SignatureRequest::type: {
                std::shared_ptr<SignatureRequest> message(mutils::from_bytes<SignatureRequest>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                message_handler->handle_message(message);
                break;
            }
            case SignatureResponse::type: {
                std::shared_ptr<SignatureResponse> message(mutils::from_bytes<SignatureResponse>(nullptr, buffer));
                buffer += mutils::bytes_size(*message);
                message_handler->handle_message(message);
                break;
            }
            default:
                logger->warn("NetworkManager dropped a message with an invalid type.");
                break;
        }
    }
}

template <typename RecordType>
bool NetworkManager<RecordType>::send(const std::list<std::shared_ptr<messaging::OverlayTransportMessage>>& messages, const int recipient_id) {
    // Construct a new socket for this node if there is not one already in the map
    auto socket_map_find = sockets_by_id.lower_bound(recipient_id);
    if(socket_map_find == sockets_by_id.end() || socket_map_find->first != recipient_id) {
        sockets_by_id.emplace_hint(socket_map_find, recipient_id,
                                   asio::ip::tcp::socket(network_io_context, id_to_ip_map.at(recipient_id)));
    }
    std::size_t send_size = mutils::bytes_size(messages.size());
    for(const auto& message : messages) {
        send_size += mutils::bytes_size(*message);
    }
    // Serialize the messages into a buffer that is stored on the heap, so it will remain in scope during the asynchronous write
    std::shared_ptr<std::vector<uint8_t>> send_buffer = std::make_shared<std::vector<uint8_t>>(sizeof(send_size) + send_size);
    std::memcpy(send_buffer->data(), send_size, sizeof(send_size));
    std::size_t bytes_written = sizeof(send_size);
    bytes_written += mutils::to_bytes(messages.size(), send_buffer->data() + bytes_written);
    for(const auto& message : messages) {
        bytes_written += mutils::to_bytes(*message, send_buffer->data() + bytes_written);
    }
    // Capture a copy of the send_buffer's shared_ptr so it stays alive until the write finishes
    asio::async_write(sockets_by_id.at(recipient_id),
                      asio::buffer(*send_buffer),
                      [this, recipient_id, send_buffer](const asio::error_code& error, std::size_t bytes_sent) {
                          handle_write_complete(recipient_id, error, bytes_sent);
                      });
    // Right now, this method assumes it's synchronous and has no way of getting a notification when the write completes,
    // so just return true even though there might be an error reported to the write handler
    return true;
}

template <typename RecordType>
bool NetworkManager<RecordType>::send(const std::shared_ptr<messaging::AggregationMessage<RecordType>>& message, const int recipient_id) {
    // Construct a new socket for this node if there is not one already in the map
    auto socket_map_find = sockets_by_id.lower_bound(recipient_id);
    if(socket_map_find == sockets_by_id.end() || socket_map_find->first != recipient_id) {
        sockets_by_id.emplace_hint(socket_map_find, recipient_id,
                                   asio::ip::tcp::socket(network_io_context, id_to_ip_map.at(recipient_id)));
    }
    const std::size_t num_messages = 1;
    std::size_t send_size = mutils::bytes_size(*message);
    //The utility doesn't need a "number of messages" header because it only accepts one message
    if(recipient_id != UTILITY_NODE_ID) {
        send_size += mutils::bytes_size(num_messages);
    }
    std::shared_ptr<std::vector<uint8_t>> send_buffer = std::make_shared<std::vector<uint8_t>>(sizeof(send_size) + send_size);
    std::memcpy(send_buffer->data(), &send_size, sizeof(send_size));
    std::size_t bytes_written = sizeof(send_size);
    if(recipient_id != UTILITY_NODE_ID) {
        bytes_written += mutils::to_bytes(num_messages, send_buffer->data() + bytes_written);
    }
    bytes_written += mutils::to_bytes(*message, send_buffer->data() + bytes_written);
    // Capture a copy of the send_buffer's shared_ptr so it stays alive until the write finishes
    asio::async_write(sockets_by_id.at(recipient_id),
                      asio::buffer(*send_buffer),
                      [this, recipient_id, send_buffer](const asio::error_code& error, std::size_t bytes_sent) {
                          handle_write_complete(recipient_id, error, bytes_sent);
                      });
    return true;
}

template<typename RecordType>
void NetworkManager<RecordType>::handle_write_complete(int recipient_id, const asio::error_code& error, std::size_t bytes_sent) {
    if(error) {
        logger->error("Write failed to complete for client {}, after sending {} bytes. Error message: {}", recipient_id, bytes_sent, error.message);
        sockets_by_id.erase(recipient_id);
    } else {
        logger->trace("Finished a write of size {} to client {}", bytes_sent, recipient_id);
    }
}

template <typename RecordType>
bool NetworkManager<RecordType>::send(const std::shared_ptr<messaging::PingMessage>& message, const int recipient_id) {
    // Construct a new socket for this node if there is not one already in the map
    auto socket_map_find = sockets_by_id.lower_bound(recipient_id);
    if(socket_map_find == sockets_by_id.end() || socket_map_find->first != recipient_id) {
        sockets_by_id.emplace_hint(socket_map_find, recipient_id,
                                   asio::ip::tcp::socket(network_io_context, id_to_ip_map.at(recipient_id)));
    }
    // Serialize the ping message
    const std::size_t num_messages = 1;
    std::size_t send_size = mutils::bytes_size(num_messages) + mutils::bytes_size(*message);
    char buffer[send_size + sizeof(send_size)];
    std::memcpy(buffer, &send_size, sizeof(send_size));
    std::size_t bytes_written = sizeof(send_size);
    bytes_written += mutils::to_bytes(num_messages, buffer + bytes_written);
    bytes_written += mutils::to_bytes(*message, buffer + bytes_written);
    // Pings are used to detect failures, so I'll use a synchronous write for now so that I can detect the errors
    asio::error_code error;
    asio::write(sockets_by_id.at(recipient_id), asio::buffer(buffer), error);
    if(error) {
        logger->debug("Failed to send ping to client {}: {}", recipient_id, error.message());
        sockets_by_id.erase(recipient_id);
        return false;
    } else {
        return true;
    }
}

template <typename RecordType>
void NetworkManager<RecordType>::run() {
    network_io_context.run();
}

template <typename RecordType>
void NetworkManager<RecordType>::shutdown() {
    network_io_context.stop();
}

}  // namespace adq