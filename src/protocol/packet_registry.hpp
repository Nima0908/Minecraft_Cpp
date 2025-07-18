#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

#include "client/status/ping_request.hpp"
#include "client/status/status_request.hpp"
#include "packet.hpp"
#include "packet_direction.hpp"
#include "packet_state.hpp"

// Clientbound
#include "client/login/cookie_response.hpp"
#include "client/login/custom_query_answer.hpp"
#include "client/login/encryption_response.hpp"
#include "client/login/login_acknowledged.hpp"
#include "client/login/login_start.hpp"

// Serverbound
#include "server/login/cookie_request.hpp"
#include "server/login/custom_query.hpp"
#include "server/login/encryption_request.hpp"
#include "server/login/login_compression.hpp"
#include "server/login/login_disconnect.hpp"
#include "server/login/login_finished.hpp"
#include "server/status/pong_response.hpp"
#include "server/status/status_response.hpp"

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
            {0x02,
             []() {
               return new mc::protocol::client::login::CustomQueryAnswer();
             }},
            {0x03,
             []() {
               return new mc::protocol::client::login::LoginAcknowledged();
             }},
            {0x04,
             []() {
               return new mc::protocol::client::login::CookieResponse();
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
             }},
            {0x04,
             []() { return new mc::protocol::server::login::CustomQuery(); }},
            {0x05,
             []() {
               return new mc::protocol::server::login::CookieRequest();
             }}}}}},
        {mc::protocol::PacketState::Configuration,
         {{mc::protocol::PacketDirection::Serverbound,
           {{0x00,
             []() {
               return new mc::protocol::client::status::StatusRequest();
             }},
            {0x01,
             []() {
               return new mc::protocol::client::status::PingRequest();
             }}}},
          {mc::protocol::PacketDirection::Clientbound,
           {{0x00,
             []() {
               return new mc::protocol::server::status::StatusResponse();
             }},
            {0x01, []() {
               return new mc::protocol::server::status::PongResponse();
             }}}}}}};
} // namespace mc::protocol
