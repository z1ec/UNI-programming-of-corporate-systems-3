#include <gtest/gtest.h>
#include "PacketCapture.h"


TEST(ICaptureStrategyContract, FreshStrategyIsNotRunning) {
    MockCaptureStrategy s;
    EXPECT_FALSE(s.isRunning());
}


TEST(ICaptureStrategyContract, ListInterfacesNonEmpty) {
    MockCaptureStrategy s;
    auto ifaces = s.listInterfaces();
    EXPECT_FALSE(ifaces.empty());
}

TEST(ICaptureStrategyContract, SelectValidInterfaceReturnsTrue) {
    MockCaptureStrategy s;
    auto ifaces = s.listInterfaces();
    ASSERT_FALSE(ifaces.empty());
    EXPECT_TRUE(s.selectInterface(0));
}


TEST(ICaptureStrategyContract, SelectNegativeIndexReturnsFalse) {
    MockCaptureStrategy s;
    EXPECT_FALSE(s.selectInterface(-1));
}


TEST(ICaptureStrategyContract, StartWithoutSelectFails) {
    MockCaptureStrategy s;
    EXPECT_FALSE(s.startCapture());
    EXPECT_FALSE(s.isRunning());
}


TEST(ICaptureStrategyContract, StartAfterSelectSucceeds) {
    MockCaptureStrategy s;
    ASSERT_TRUE(s.selectInterface(0));
    EXPECT_TRUE(s.startCapture());
    EXPECT_TRUE(s.isRunning());
}


TEST(ICaptureStrategyContract, StopCaptureStopsRunning) {
    MockCaptureStrategy s;
    ASSERT_TRUE(s.selectInterface(0));
    ASSERT_TRUE(s.startCapture());
    s.stopCapture();
    EXPECT_FALSE(s.isRunning());
}


TEST(CaptureStrategyFactory, ReturnsNonNull) {
    auto strategy = createCaptureStrategy();
    EXPECT_NE(strategy, nullptr);
}


TEST(CaptureStrategyFactory, FactoryStrategyCanListInterfaces) {
    auto strategy = createCaptureStrategy();
    ASSERT_NE(strategy, nullptr);
    auto ifaces = strategy->listInterfaces();
    EXPECT_FALSE(ifaces.empty());
}


#ifdef USE_PCAP
#include "PcapCaptureStrategy.h"


TEST(PcapCaptureStrategyTest, FreshIsNotRunning) {
    PcapCaptureStrategy s;
    EXPECT_FALSE(s.isRunning());
}


TEST(PcapCaptureStrategyTest, SelectOutOfRangeFails) {
    PcapCaptureStrategy s;
    auto ifaces = s.listInterfaces(); 
    (void)ifaces;
    EXPECT_FALSE(s.selectInterface(9999));

    const std::string& err = s.lastError();
    (void)err;
}


TEST(PcapCaptureStrategyTest, GetNextWithoutStartReturnsNullopt) {
    PcapCaptureStrategy s;
    EXPECT_EQ(s.getNextPacket(), std::nullopt);
}

#endif // USE_PCAP
