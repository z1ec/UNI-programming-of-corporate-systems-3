#include <gtest/gtest.h>
#include "PacketCapture.h"

// ---------------------------------------------------------------------------
// ICaptureStrategy interface contract tests
//
// These tests verify the contract every concrete strategy must uphold.
// We use MockCaptureStrategy as a reference implementation.  When compiled
// with USE_PCAP, an additional section tests PcapCaptureStrategy directly.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 1. Interface contract: fresh strategy is not running
// ---------------------------------------------------------------------------
TEST(ICaptureStrategyContract, FreshStrategyIsNotRunning) {
    MockCaptureStrategy s;
    EXPECT_FALSE(s.isRunning());
}

// ---------------------------------------------------------------------------
// 2. Interface contract: listInterfaces() returns a non-empty list
// ---------------------------------------------------------------------------
TEST(ICaptureStrategyContract, ListInterfacesNonEmpty) {
    MockCaptureStrategy s;
    auto ifaces = s.listInterfaces();
    EXPECT_FALSE(ifaces.empty());
}

// ---------------------------------------------------------------------------
// 3. Interface contract: selectInterface with valid index returns true
// ---------------------------------------------------------------------------
TEST(ICaptureStrategyContract, SelectValidInterfaceReturnsTrue) {
    MockCaptureStrategy s;
    auto ifaces = s.listInterfaces();
    ASSERT_FALSE(ifaces.empty());
    EXPECT_TRUE(s.selectInterface(0));
}

// ---------------------------------------------------------------------------
// 4. Interface contract: selectInterface with negative index returns false
// ---------------------------------------------------------------------------
TEST(ICaptureStrategyContract, SelectNegativeIndexReturnsFalse) {
    MockCaptureStrategy s;
    EXPECT_FALSE(s.selectInterface(-1));
}

// ---------------------------------------------------------------------------
// 5. Interface contract: startCapture without prior selectInterface fails
// ---------------------------------------------------------------------------
TEST(ICaptureStrategyContract, StartWithoutSelectFails) {
    MockCaptureStrategy s;
    EXPECT_FALSE(s.startCapture());
    EXPECT_FALSE(s.isRunning());
}

// ---------------------------------------------------------------------------
// 6. Interface contract: startCapture after selectInterface succeeds
// ---------------------------------------------------------------------------
TEST(ICaptureStrategyContract, StartAfterSelectSucceeds) {
    MockCaptureStrategy s;
    ASSERT_TRUE(s.selectInterface(0));
    EXPECT_TRUE(s.startCapture());
    EXPECT_TRUE(s.isRunning());
}

// ---------------------------------------------------------------------------
// 7. Interface contract: stopCapture brings isRunning to false
// ---------------------------------------------------------------------------
TEST(ICaptureStrategyContract, StopCaptureStopsRunning) {
    MockCaptureStrategy s;
    ASSERT_TRUE(s.selectInterface(0));
    ASSERT_TRUE(s.startCapture());
    s.stopCapture();
    EXPECT_FALSE(s.isRunning());
}

// ---------------------------------------------------------------------------
// 8. createCaptureStrategy factory returns a non-null pointer
// ---------------------------------------------------------------------------
TEST(CaptureStrategyFactory, ReturnsNonNull) {
    auto strategy = createCaptureStrategy();
    EXPECT_NE(strategy, nullptr);
}

// ---------------------------------------------------------------------------
// 9. Strategy returned by factory honours listInterfaces()
// ---------------------------------------------------------------------------
TEST(CaptureStrategyFactory, FactoryStrategyCanListInterfaces) {
    auto strategy = createCaptureStrategy();
    ASSERT_NE(strategy, nullptr);
    auto ifaces = strategy->listInterfaces();
    EXPECT_FALSE(ifaces.empty());
}

// ---------------------------------------------------------------------------
// PcapCaptureStrategy tests (only when compiled with libpcap)
// ---------------------------------------------------------------------------
#ifdef USE_PCAP
#include "PcapCaptureStrategy.h"

// 10. PcapCaptureStrategy: fresh instance is not running
TEST(PcapCaptureStrategyTest, FreshIsNotRunning) {
    PcapCaptureStrategy s;
    EXPECT_FALSE(s.isRunning());
}

// 11. PcapCaptureStrategy: selectInterface with out-of-range index fails
TEST(PcapCaptureStrategyTest, SelectOutOfRangeFails) {
    PcapCaptureStrategy s;
    auto ifaces = s.listInterfaces(); // populate internal device list
    (void)ifaces;
    EXPECT_FALSE(s.selectInterface(9999));
    // lastError() must be accessible and return a string (any value)
    const std::string& err = s.lastError();
    (void)err;
}

// 12. PcapCaptureStrategy: getNextPacket without startCapture returns nullopt
TEST(PcapCaptureStrategyTest, GetNextWithoutStartReturnsNullopt) {
    PcapCaptureStrategy s;
    EXPECT_EQ(s.getNextPacket(), std::nullopt);
}

#endif // USE_PCAP
