#pragma once

#include <string>
#include <chrono>
#include <cstdint>

// ---------------------------------------------------------------------------
// Protocol – all layer-3/4 protocol types the analyzer recognizes.
// ---------------------------------------------------------------------------
enum class Protocol {
    TCP,
    UDP,
    ICMP,
    ICMPv6,
    IPv4,   // generic IPv4 (no recognized transport)
    IPv6,   // generic IPv6 (no recognized transport)
    UNKNOWN
};

// Convert a Protocol value to a human-readable string.
std::string protocolToString(Protocol p);

// ---------------------------------------------------------------------------
// PacketInfo – the canonical representation of one captured packet after it
// has been parsed.  Every other subsystem works with this struct.
// ---------------------------------------------------------------------------
struct PacketInfo {
    std::string sourceIp;
    std::string destinationIp;
    Protocol    protocol   = Protocol::UNKNOWN;
    std::size_t sizeBytes  = 0;
    std::chrono::system_clock::time_point timestamp;
};
