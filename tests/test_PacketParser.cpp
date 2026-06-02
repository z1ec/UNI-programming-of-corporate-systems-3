#include <gtest/gtest.h>
#include "PacketParser.h"
#include "PacketCapture.h"  
#include "PacketInfo.h"


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


RawPacket makeRealIPv4(uint8_t proto,
                       const uint8_t src[4], const uint8_t dst[4])
{
    std::vector<uint8_t> frame;

  
    frame.insert(frame.end(), 6, 0x00);           
    frame.insert(frame.end(), 6, 0x00);           
    frame.push_back(0x08); frame.push_back(0x00); 

 
    frame.push_back(0x45);       
    frame.push_back(0x00);  
    frame.push_back(0x00); frame.push_back(0x28); 
    frame.push_back(0x00); frame.push_back(0x01); 
    frame.push_back(0x00); frame.push_back(0x00); 
    frame.push_back(0x40);        
    frame.push_back(proto);       
    frame.push_back(0x00); frame.push_back(0x00); 
    frame.insert(frame.end(), src, src + 4);       
    frame.insert(frame.end(), dst, dst + 4);       

    RawPacket p;
    p.isMock = false;
    p.data   = frame;
    p.length = frame.size();
    return p;
}


RawPacket makeRealIPv6(uint8_t nextHeader,
                       const uint8_t src[16], const uint8_t dst[16])
{
    std::vector<uint8_t> frame;


    frame.insert(frame.end(), 6, 0x00);
    frame.insert(frame.end(), 6, 0x00);
    frame.push_back(0x86); frame.push_back(0xDD); 


    frame.push_back(0x60); frame.push_back(0x00);  
    frame.push_back(0x00); frame.push_back(0x00);  
    frame.push_back(0x00); frame.push_back(0x00);  
    frame.push_back(nextHeader);                    
    frame.push_back(0x40);                          
    frame.insert(frame.end(), src, src + 16);       
    frame.insert(frame.end(), dst, dst + 16);       

    RawPacket p;
    p.isMock = false;
    p.data   = frame;
    p.length = frame.size();
    return p;
}

} 



class PacketParserTest : public ::testing::Test {
protected:
    PacketParser parser;
};


TEST_F(PacketParserTest, MockTcpPacket) {
    auto raw = makeMock("10.0.0.1", "10.0.0.2", 6, 1500);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol,      Protocol::TCP);
    EXPECT_EQ(info.sourceIp,      "10.0.0.1");
    EXPECT_EQ(info.destinationIp, "10.0.0.2");
    EXPECT_EQ(info.sizeBytes,     1500u);
}


TEST_F(PacketParserTest, MockUdpPacket) {
    auto raw = makeMock("192.168.1.1", "8.8.8.8", 17, 64);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol,      Protocol::UDP);
    EXPECT_EQ(info.sourceIp,      "192.168.1.1");
    EXPECT_EQ(info.destinationIp, "8.8.8.8");
}


TEST_F(PacketParserTest, MockIcmpPacket) {
    auto raw = makeMock("172.16.0.1", "172.16.0.2", 1, 84);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol, Protocol::ICMP);
}


TEST_F(PacketParserTest, MockIcmpv6Packet) {
    auto raw = makeMock("::1", "::2", 58, 100);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol, Protocol::ICMPv6);
}


TEST_F(PacketParserTest, MockUnknownProtocol) {
    auto raw = makeMock("1.2.3.4", "5.6.7.8", 253, 40);
    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol, Protocol::UNKNOWN);
}


TEST_F(PacketParserTest, RealIPv4TcpFrame) {
    const uint8_t src[4] = {192, 168, 1, 10};
    const uint8_t dst[4] = {8,   8,   8,  8};
    auto raw = makeRealIPv4(6, src, dst);

    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol,      Protocol::TCP);
    EXPECT_EQ(info.sourceIp,      "192.168.1.10");
    EXPECT_EQ(info.destinationIp, "8.8.8.8");
}


TEST_F(PacketParserTest, RealPacketTooShort) {
    RawPacket p;
    p.isMock = false;
    p.data   = {0x00, 0x01, 0x02}; // only 3 bytes – far too short
    p.length = 3;

    PacketInfo info = parser.parse(p);

    EXPECT_EQ(info.protocol, Protocol::UNKNOWN);
}


TEST_F(PacketParserTest, RealIPv6Icmpv6Frame) {
    const uint8_t src[16] = {0xfe,0x80,0,0,0,0,0,0, 0,0,0,0,0,0,0,1};
    const uint8_t dst[16] = {0xfe,0x80,0,0,0,0,0,0, 0,0,0,0,0,0,0,2};
    auto raw = makeRealIPv6(58, src, dst);

    PacketInfo info = parser.parse(raw);

    EXPECT_EQ(info.protocol, Protocol::ICMPv6);
}


TEST_F(PacketParserTest, ProtocolToStringAllValues) {
    EXPECT_EQ(protocolToString(Protocol::TCP),     "TCP");
    EXPECT_EQ(protocolToString(Protocol::UDP),     "UDP");
    EXPECT_EQ(protocolToString(Protocol::ICMP),    "ICMP");
    EXPECT_EQ(protocolToString(Protocol::ICMPv6),  "ICMPv6");
    EXPECT_EQ(protocolToString(Protocol::IPv4),    "IPv4");
    EXPECT_EQ(protocolToString(Protocol::IPv6),    "IPv6");
    EXPECT_EQ(protocolToString(Protocol::UNKNOWN), "UNKNOWN");
}
