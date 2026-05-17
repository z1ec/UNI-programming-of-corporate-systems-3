#include "PacketParser.h"

#include <chrono>
#include <cstdint>
#include <cstring>

// Platform-specific header for inet_ntop (binary IP → human-readable string).
#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#endif

// ============================================================================
//  Wire-format constants
// ============================================================================
namespace {

constexpr std::size_t ETH_HEADER_LEN = 14;    // 6 dst MAC + 6 src MAC + 2 ethertype
constexpr uint16_t    ETHERTYPE_IPV4 = 0x0800;
constexpr uint16_t    ETHERTYPE_IPV6 = 0x86DD;
constexpr std::size_t IPV4_MIN_HDR   = 20;
constexpr std::size_t IPV6_HDR_LEN   = 40;

// Read a big-endian uint16_t from two consecutive bytes.
inline uint16_t readBE16(const uint8_t* p) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(p[0]) << 8) | p[1]
    );
}

// ---------------------------------------------------------------------------
// parseIPv4 – fill sourceIp / destinationIp in `out` from an IPv4 header.
//
// Returns the IANA transport protocol number (field [9]), or -1 on error.
// Intentionally does NOT call PacketParser::mapProtocol (private); the caller
// (PacketParser::parse, a member function) does that conversion.
// ---------------------------------------------------------------------------
int parseIPv4(const uint8_t* ip, std::size_t avail, PacketInfo& out) {
    if (avail < IPV4_MIN_HDR) { return -1; }

    const std::size_t ihl = static_cast<std::size_t>(ip[0] & 0x0Fu) * 4u;
    if (ihl < IPV4_MIN_HDR || ihl > avail) { return -1; }

    char srcBuf[INET_ADDRSTRLEN]{};
    char dstBuf[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, ip + 12, srcBuf, sizeof(srcBuf));
    inet_ntop(AF_INET, ip + 16, dstBuf, sizeof(dstBuf));

    out.sourceIp      = srcBuf;
    out.destinationIp = dstBuf;
    return static_cast<int>(ip[9]);  // IANA protocol number
}

// ---------------------------------------------------------------------------
// parseIPv6 – fill sourceIp / destinationIp in `out` from an IPv6 header.
//
// Returns the IANA next-header value (field [6]), or -1 on error.
// ---------------------------------------------------------------------------
int parseIPv6(const uint8_t* ip, std::size_t avail, PacketInfo& out) {
    if (avail < IPV6_HDR_LEN) { return -1; }

    char srcBuf[INET6_ADDRSTRLEN]{};
    char dstBuf[INET6_ADDRSTRLEN]{};
    inet_ntop(AF_INET6, ip + 8,  srcBuf, sizeof(srcBuf));
    inet_ntop(AF_INET6, ip + 24, dstBuf, sizeof(dstBuf));

    out.sourceIp      = srcBuf;
    out.destinationIp = dstBuf;
    return static_cast<int>(ip[6]);  // next-header (same numbering as IANA)
}

} // anonymous namespace

// ============================================================================
//  protocolToString  (free function declared in PacketInfo.h)
// ============================================================================

std::string protocolToString(Protocol p) {
    switch (p) {
        case Protocol::TCP:     return "TCP";
        case Protocol::UDP:     return "UDP";
        case Protocol::ICMP:    return "ICMP";
        case Protocol::ICMPv6:  return "ICMPv6";
        case Protocol::IPv4:    return "IPv4";
        case Protocol::IPv6:    return "IPv6";
        case Protocol::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

// ============================================================================
//  PacketParser – private helpers
// ============================================================================

Protocol PacketParser::mapProtocol(int protocolNumber) {
    switch (protocolNumber) {
        case 1:  return Protocol::ICMP;
        case 6:  return Protocol::TCP;
        case 17: return Protocol::UDP;
        case 41: return Protocol::IPv6;   // IPv6-in-IPv4 encapsulation
        case 58: return Protocol::ICMPv6;
        default: return Protocol::UNKNOWN;
    }
}

// ============================================================================
//  PacketParser::parse  – public entry point
// ============================================================================

PacketInfo PacketParser::parse(const RawPacket& raw) const {
    PacketInfo info;
    info.timestamp = std::chrono::system_clock::now();
    info.sizeBytes = raw.length;

    // ── Mock mode: use pre-filled simulation fields ──────────────────────
    if (raw.isMock) {
        info.sourceIp      = raw.simulatedSourceIp;
        info.destinationIp = raw.simulatedDestIp;
        info.protocol      = mapProtocol(raw.simulatedProtocol);
        info.sizeBytes     = raw.simulatedSize;
        return info;
    }

    // ── Real mode: decode Ethernet II + IP headers from raw.data ─────────
    //
    // Frame layout:
    //   [0..5]   destination MAC
    //   [6..11]  source MAC
    //   [12..13] EtherType   (0x0800 = IPv4, 0x86DD = IPv6)
    //   [14..]   IP header + payload
    //
    const std::vector<uint8_t>& buf = raw.data;

    if (buf.size() < ETH_HEADER_LEN) {
        info.protocol = Protocol::UNKNOWN;
        return info;
    }

    const uint16_t  etherType = readBE16(buf.data() + 12);
    const uint8_t*  ipStart   = buf.data() + ETH_HEADER_LEN;
    const std::size_t ipAvail = buf.size() - ETH_HEADER_LEN;

    // protoNum: IANA number returned by the IP-layer parser, or -1 on error.
    int  protoNum = -1;
    bool isIPv6   = false;

    if (etherType == ETHERTYPE_IPV4) {
        protoNum = parseIPv4(ipStart, ipAvail, info);
    } else if (etherType == ETHERTYPE_IPV6) {
        protoNum = parseIPv6(ipStart, ipAvail, info);
        isIPv6   = true;
    }

    if (protoNum < 0) {
        // Malformed or unsupported EtherType.
        info.sourceIp      = "?";
        info.destinationIp = "?";
        info.protocol      = Protocol::UNKNOWN;
    } else {
        // mapProtocol is a private member; calling it here (in parse(), also a
        // member) is valid.
        info.protocol = mapProtocol(protoNum);

        // If the transport protocol is not one we track individually, fall back
        // to the generic IP version label so the frame still gets counted.
        if (info.protocol == Protocol::UNKNOWN) {
            info.protocol = isIPv6 ? Protocol::IPv6 : Protocol::IPv4;
        }
    }

    return info;
}
