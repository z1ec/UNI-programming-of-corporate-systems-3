#pragma once

#include "PacketInfo.h"
#include "PacketCapture.h"

// ---------------------------------------------------------------------------
// PacketParser – translates a RawPacket into a structured PacketInfo.
//
// In mock mode it reads the pre-filled simulation fields.
// In real pcap mode it would inspect the raw byte buffer to decode Ethernet,
// IPv4/IPv6, and transport-layer headers.
// ---------------------------------------------------------------------------
class PacketParser {
public:
    // Parse one raw packet and return a fully populated PacketInfo.
    [[nodiscard]] PacketInfo parse(const RawPacket& raw) const;

private:
    // Map an IANA protocol number to the internal Protocol enum.
    [[nodiscard]] static Protocol mapProtocol(int protocolNumber);
};
