#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

#include "packet.hpp"
#include "packet_state.hpp"

// Clientbound
#include "client/login/encryption_response.hpp"
#include "client/login/login_acknowledged.hpp"
#include "client/login/login_start.hpp"

// Serverbound
#include "server/login/encryption_request.hpp"
#include "server/login/login_compression.hpp"
#include "server/login/login_disconnect.hpp"
#include "server/login/login_finished.hpp"

namespace mc::protocol {

using PacketFactory = std::function<mc::protocol::Packet *()>;

// Nested map:
// PacketState -> PacketDirection -> Packet ID -> PacketFactory
inline const std::unordered_map<
    mc::protocol::PacketState,
    std::unordered_map<mc::protocol::PacketDirection,
                       std::unordered_map<uint8_t, PacketFactory>>>
    packetFactoryRegistry = {
        {mc::protocol::PacketState::Login,
         {{mc::protocol::PacketDirection::Serverbound,
           {{0x00,
             []() { return new mc::protocol::client::login::LoginStart(); }},
            {0x01,
             []() {
               return new mc::protocol::client::login::EncryptionResponse();
             }},
            {0x03,
             []() {
               return new mc::protocol::client::login::LoginAcknowledged();
             }}}},
          {mc::protocol::PacketDirection::Clientbound,
           {{0x00,
             []() {
               return new mc::protocol::server::login::LoginDisconnect();
             }},
            {0x01,
             []() {
               return new mc::protocol::server::login::EncryptionRequest();
             }},
            {0x02,
             []() { return new mc::protocol::server::login::LoginFinished(); }},
            {0x03,
             []() {
               return new mc::protocol::server::login::LoginCompression();
             }}}}}},
};
} // namespace mc::protocol
