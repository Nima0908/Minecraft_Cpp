#pragma once
#include "../packet.hpp"
#include "../../util/buffer_util.hpp"
#include "../../network/socket.hpp"
#include <string>
#include <vector>
#include <array>
#include <optional>

namespace mc {

class LoginFinished : public Packet {
public:
    std::array<uint8_t, 16> uuid;
    std::string username;
    
    struct Property {
        std::string name;
        std::string value;
        std::optional<std::string> signature;
    };
    std::vector<Property> properties;
    
    LoginFinished() = default;
    
    std::vector<uint8_t> serialize() const override {
        return {};  
    }
    
    uint32_t getPacketID() const override {
        return 0x02;
    }
    
    PacketDirection getDirection() const override {
        return PacketDirection::Clientbound;
    }
    
    void read(Socket& socket) {
        // Read UUID (16 bytes)
        //std::vector<uint8_t> uuidBytes = socket.recv(16);
        //if (uuidBytes.size() != 16) {
        //    throw std::runtime_error("Invalid UUID length: expected 16 bytes, got " + std::to_string(uuidBytes.size()));
        //}
        //std::copy(uuidBytes.begin(), uuidBytes.end(), uuid.begin());
        
        // Read username
        username = socket.recvString();
        
        // Read number of properties
        int32_t numProperties = socket.recvVarInt();
        properties.clear();
        properties.reserve(numProperties);
        
        // Read each property
        for (int32_t i = 0; i < numProperties; i++) {
            Property prop;
            prop.name = socket.recvString();
            prop.value = socket.recvString();
            
            // Check if signature is present
            bool hasSignature = socket.recvByte() != 0;
            if (hasSignature) {
                prop.signature = socket.recvString();
            }
            
            properties.push_back(std::move(prop));
        }
    }
};

}  // namespace mc
