#include <gtest/gtest.h>
#include "TrafficStatistics.h"
#include "PacketInfo.h"

namespace {

PacketInfo makePacket(const std::string& src, const std::string& dst,
                      Protocol proto, std::size_t bytes)
{
    PacketInfo p;
    p.sourceIp      = src;
    p.destinationIp = dst;
    p.protocol      = proto;
    p.sizeBytes     = bytes;
    return p;
}

} 

class TrafficStatisticsTest : public ::testing::Test {
protected:
    TrafficStatistics stats;
};


TEST_F(TrafficStatisticsTest, InitiallyEmpty) {
    EXPECT_EQ(stats.getTotalPackets(), 0u);
    EXPECT_EQ(stats.getTotalBytes(),   0u);
    EXPECT_TRUE(stats.getIpStats().empty());
    EXPECT_TRUE(stats.getProtocolStats().empty());
}


TEST_F(TrafficStatisticsTest, AddPacketIncrementsTotalCount) {
    stats.addPacket(makePacket("1.1.1.1", "2.2.2.2", Protocol::TCP, 100));
    EXPECT_EQ(stats.getTotalPackets(), 1u);

    stats.addPacket(makePacket("1.1.1.1", "3.3.3.3", Protocol::UDP, 50));
    EXPECT_EQ(stats.getTotalPackets(), 2u);
}


TEST_F(TrafficStatisticsTest, AddPacketAccumulatesBytes) {
    stats.addPacket(makePacket("1.1.1.1", "2.2.2.2", Protocol::TCP,  400));
    stats.addPacket(makePacket("1.1.1.1", "2.2.2.2", Protocol::TCP,  600));
    EXPECT_EQ(stats.getTotalBytes(), 1000u);
}


TEST_F(TrafficStatisticsTest, SameSourceIpAggregated) {
    stats.addPacket(makePacket("10.0.0.1", "10.0.0.2", Protocol::TCP, 100));
    stats.addPacket(makePacket("10.0.0.1", "10.0.0.3", Protocol::UDP, 200));

    const auto& ip = stats.getIpStats();
    ASSERT_EQ(ip.count("10.0.0.1"), 1u);
    EXPECT_EQ(ip.at("10.0.0.1").packetCount, 2u);
    EXPECT_EQ(ip.at("10.0.0.1").totalBytes,  300u);
}


TEST_F(TrafficStatisticsTest, ProtocolCountsSeparated) {
    stats.addPacket(makePacket("a", "b", Protocol::TCP,  100));
    stats.addPacket(makePacket("a", "b", Protocol::TCP,  100));
    stats.addPacket(makePacket("a", "b", Protocol::UDP,   50));
    stats.addPacket(makePacket("a", "b", Protocol::ICMP,  84));

    const auto& proto = stats.getProtocolStats();
    EXPECT_EQ(proto.at(Protocol::TCP),  2u);
    EXPECT_EQ(proto.at(Protocol::UDP),  1u);
    EXPECT_EQ(proto.at(Protocol::ICMP), 1u);
}


TEST_F(TrafficStatisticsTest, ResetClearsEverything) {
    stats.addPacket(makePacket("1.2.3.4", "5.6.7.8", Protocol::TCP, 500));
    stats.reset();

    EXPECT_EQ(stats.getTotalPackets(), 0u);
    EXPECT_EQ(stats.getTotalBytes(),   0u);
    EXPECT_TRUE(stats.getIpStats().empty());
    EXPECT_TRUE(stats.getProtocolStats().empty());
}


TEST_F(TrafficStatisticsTest, ZeroBytesPacketCounted) {
    stats.addPacket(makePacket("0.0.0.0", "0.0.0.1", Protocol::UNKNOWN, 0));
    EXPECT_EQ(stats.getTotalPackets(), 1u);
    EXPECT_EQ(stats.getTotalBytes(),   0u);
}


TEST_F(TrafficStatisticsTest, MultipleSourceIpsTrackedSeparately) {
    stats.addPacket(makePacket("1.1.1.1", "9.9.9.9", Protocol::TCP, 100));
    stats.addPacket(makePacket("2.2.2.2", "9.9.9.9", Protocol::TCP, 200));
    stats.addPacket(makePacket("3.3.3.3", "9.9.9.9", Protocol::TCP, 300));

    const auto& ip = stats.getIpStats();
    EXPECT_EQ(ip.size(), 3u);
    EXPECT_EQ(ip.at("1.1.1.1").totalBytes, 100u);
    EXPECT_EQ(ip.at("2.2.2.2").totalBytes, 200u);
    EXPECT_EQ(ip.at("3.3.3.3").totalBytes, 300u);
}
