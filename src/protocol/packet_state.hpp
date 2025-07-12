#pragma once

namespace mc::protocol {

enum class PacketState { Handshaking, Status, Login, Configuration, Play };
}
