#include <gtest/gtest.h>
#include "PacketParser.h"
#include "PacketCapture.h"  // RawPacket
#include "PacketInfo.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

RawPacket makeMock(const std::string& src, const std::string& dst,
                   int proto, std::size_t size)
{
    RawPacket p;
    p.isMock             = true;
    p.simulatedSourceIp  = src;
    p.simulatedDestIp    = dst;
    p.simulatedProtocol  = proto;
    p.simulatedSize      = size;
    p.length             = size;
    return p;
}

// Build a minimal Ethernet-II + IPv4 frame in network byte order.
// proto: IANA number (1=ICMP, 6=TCP, 17=UDP).
RawPacket makeRealIPv4(uint8_t proto,
                       const uint8_t src[4], const uint8_t dst[4])
{
    std::vector<uint8_t> frame;

    // Ethernet header (14 bytes)
    frame.insert(frame.end(), 6, 0x00);           // dst MAC
    frame.insert(frame.end(), 6, 0x00);           // src MAC
    frame.push_back(0x08); frame.push_back(0x00); // EtherType = IPv4

    // IPv4 header (20 bytes, IHL = 5)
    frame.push_back(0x45);        // Version=4, IHL=5
    frame.push_back(0x00);        // DSCP/ECN
    frame.push_back(0x00); frame.push_back(0x28); // total length = 40
    frame.push_back(0x00); frame.push_back(0x01); // identification
    frame.push_back(0x00); frame.push_back(0x00); // flags/fragment offset
    frame.push_back(0x40);        // TTL = 64
    frame.push_back(proto);       // protocol
    frame.push_back(0x00); frame.push_back(0x00); // header checksum (unused)
    frame.insert(frame.end(), src, src + 4);       // source IP
    frame.insert(frame.end(), dst, dst + 4);       // destination IP

    RawPacket p;
    p.isMock = false;
    p.data   = frame;
    p.length = frame.size();
    return p;
}

// Build an Ethernet-II + IPv6 frame (minimal, 54 bytes).
RawPacket makeRealIPv6(uint8_t nextHeader,
                       const uint8_t src[16], const uint8_t dst[16])
{
    std::vector<uint8_t> frame;

    // Ethernet header
    frame.insert(frame.end(), 6, 0x00);
    frame.insert(frame.end(), 6, 0x00);
    frame.push_back(0x86); frame.push_back(0xDD); // EtherType = IPv6

    // IPv6 header (40 bytes)
    frame.push_back(0x60); frame.push_back(0x00);  // version=6, TC, Flow[0..1]
    frame.push_back(0x00); frame.push_back(0x00);  // Flow label remainder
    frame.push_back(0x00); frame.push_back(0x00);  // payload length
    frame.push_back(nextHeader);                    // next header
    frame.push_back(0x40);                          // hop limit = 64
    frame.insert(frame.end(), src, src + 16);       // source address
    frame.insert(frame.end(), dst, dst + 16);       // destination address

    RawPacket p;
    p.isMock = false;
    p.data   = frame;
    p.length = frame.size();
    return p;
}

} // namespace

// ---------------------------------------------------------------------------
// PacketParser tests
// ---------------------------------------------------------------------------

class PacketParserTest : public ::testing::Test {
protected:
    PacketParser parser;
};

// 1. Mock TCP packet → Protocol::TCP, correct IPs and size
TEST_F(PacketParserTest, MockTcpPacket) {
    auto raw = makeMock("10.0.0.1", "10.0.0.2", 6, 1500);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol,      Protocol::TCP);
    EXPECT_EQ(info.sourceIp,      "10.0.0.1");
    EXPECT_EQ(info.destinationIp, "10.0.0.2");
    EXPECT_EQ(info.sizeBytes,     1500u);
}

// 2. Mock UDP packet → Protocol::UDP
TEST_F(PacketParserTest, MockUdpPacket) {
    auto raw = makeMock("192.168.1.1", "8.8.8.8", 17, 64);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol,      Protocol::UDP);
    EXPECT_EQ(info.sourceIp,      "192.168.1.1");
    EXPECT_EQ(info.destinationIp, "8.8.8.8");
}

// 3. Mock ICMP packet → Protocol::ICMP
TEST_F(PacketParserTest, MockIcmpPacket) {
    auto raw = makeMock("172.16.0.1", "172.16.0.2", 1, 84);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol, Protocol::ICMP);
}

// 4. Mock ICMPv6 packet (protocol 58) → Protocol::ICMPv6
TEST_F(PacketParserTest, MockIcmpv6Packet) {
    auto raw = makeMock("::1", "::2", 58, 100);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol, Protocol::ICMPv6);
}

// 5. Mock packet with unsupported/unknown protocol number
TEST_F(PacketParserTest, MockUnknownProtocol) {
    auto raw = makeMock("1.2.3.4", "5.6.7.8", 253, 40);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol, Protocol::UNKNOWN);
}

// 6. Real IPv4 TCP frame – parser decodes IPs and protocol from raw bytes
TEST_F(PacketParserTest, RealIPv4TcpFrame) {
    const uint8_t src[4] = {192, 168, 1, 10};
    const uint8_t dst[4] = {8,   8,   8,  8};
    auto raw = makeRealIPv4(6, src, dst);

    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol,      Protocol::TCP);
    EXPECT_EQ(info.sourceIp,      "192.168.1.10");
    EXPECT_EQ(info.destinationIp, "8.8.8.8");
}

// 7. Real packet with too-short data (less than Ethernet header) → UNKNOWN
TEST_F(PacketParserTest, RealPacketTooShort) {
    RawPacket p;
    p.isMock = false;
    p.data   = {0x00, 0x01, 0x02}; // only 3 bytes – far too short
    p.length = 3;

    PacketInfo info = parser.parse(p);

    EXPECT_EQ(info.protocol, Protocol::UNKNOWN);
}

// 8. Real IPv6 ICMPv6 frame – next-header 58 → ICMPv6
TEST_F(PacketParserTest, RealIPv6Icmpv6Frame) {
    const uint8_t src[16] = {0xfe,0x80,0,0,0,0,0,0, 0,0,0,0,0,0,0,1};
    const uint8_t dst[16] = {0xfe,0x80,0,0,0,0,0,0, 0,0,0,0,0,0,0,2};
    auto raw = makeRealIPv6(58, src, dst);

    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol, Protocol::ICMPv6);
}

// 9. protocolToString covers all enum values
TEST_F(PacketParserTest, ProtocolToStringAllValues) {
    EXPECT_EQ(protocolToString(Protocol::TCP),     "TCP");
    EXPECT_EQ(protocolToString(Protocol::UDP),     "UDP");
    EXPECT_EQ(protocolToString(Protocol::ICMP),    "ICMP");
    EXPECT_EQ(protocolToString(Protocol::ICMPv6),  "ICMPv6");
    EXPECT_EQ(protocolToString(Protocol::IPv4),    "IPv4");
    EXPECT_EQ(protocolToString(Protocol::IPv6),    "IPv6");
    EXPECT_EQ(protocolToString(Protocol::UNKNOWN), "UNKNOWN");
}
