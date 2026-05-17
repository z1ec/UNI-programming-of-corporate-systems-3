#include "TrafficStatistics.h"

// ============================================================================
//  TrafficStatistics
// ============================================================================

void TrafficStatistics::addPacket(const PacketInfo& packet) {
    // Per-IP counters (keyed by source address).
    auto& ipEntry       = ipStats_[packet.sourceIp];
    ipEntry.packetCount += 1;
    ipEntry.totalBytes  += packet.sizeBytes;

    // Per-protocol counters.
    protocolStats_[packet.protocol] += 1;

    // Global totals.
    totalBytes_   += packet.sizeBytes;
    totalPackets_ += 1;
}

const std::map<std::string, IpStats>& TrafficStatistics::getIpStats() const {
    return ipStats_;
}

const std::map<Protocol, std::size_t>& TrafficStatistics::getProtocolStats() const {
    return protocolStats_;
}

std::size_t TrafficStatistics::getTotalBytes() const {
    return totalBytes_;
}

std::size_t TrafficStatistics::getTotalPackets() const {
    return totalPackets_;
}

void TrafficStatistics::reset() {
    ipStats_.clear();
    protocolStats_.clear();
    totalBytes_   = 0;
    totalPackets_ = 0;
}
