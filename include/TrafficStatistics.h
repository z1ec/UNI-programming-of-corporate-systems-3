#pragma once

#include "PacketInfo.h"
#include <map>
#include <string>
#include <cstdint>

// Per-IP address counters.
struct IpStats {
    std::size_t packetCount = 0;
    std::size_t totalBytes  = 0;
};

// ---------------------------------------------------------------------------
// TrafficStatistics – accumulates packet data during a capture session.
//
// Stores:
//   • per-source-IP   packet counts and byte totals
//   • per-protocol    packet counts
//   • global totals
//
// Thread-safety: single-threaded use only (no mutex).
// ---------------------------------------------------------------------------
class TrafficStatistics {
public:
    // Register one parsed packet with all counters.
    void addPacket(const PacketInfo& packet);

    // Read-only accessors ──────────────────────────────────────────────────
    [[nodiscard]] const std::map<std::string, IpStats>&  getIpStats()       const;
    [[nodiscard]] const std::map<Protocol, std::size_t>& getProtocolStats() const;
    [[nodiscard]] std::size_t                            getTotalBytes()    const;
    [[nodiscard]] std::size_t                            getTotalPackets()  const;

    // Reset all counters (e.g. before a new capture session).
    void reset();

private:
    std::map<std::string, IpStats>  ipStats_;
    std::map<Protocol, std::size_t> protocolStats_;
    std::size_t totalBytes_   = 0;
    std::size_t totalPackets_ = 0;
};
