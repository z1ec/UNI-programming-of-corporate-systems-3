#include <gtest/gtest.h>
#include "PacketCapture.h"

// ---------------------------------------------------------------------------
// Lightweight test strategy – same interface as MockCaptureStrategy
// but returns only N packets with zero delay and accepts any non-negative index.
// ---------------------------------------------------------------------------
class FastTestStrategy : public ICaptureStrategy {
public:
    explicit FastTestStrategy(int maxPkts = 5, int maxIfaces = 3)
        : maxPkts_(maxPkts), maxIfaces_(maxIfaces) {}

    std::vector<std::string> listInterfaces() override {
        std::vector<std::string> ifaces;
        for (int i = 0; i < maxIfaces_; ++i) {
            ifaces.push_back("test_iface" + std::to_string(i));
        }
        return ifaces;
    }

    bool selectInterface(int i) override {
        if (i < 0 || i >= maxIfaces_) return false;
        selectedIface_ = i;
        return true;
    }

    bool startCapture() override {
        if (selectedIface_ < 0) return false;
        running_ = true;
        sent_    = 0;
        return true;
    }

    void stopCapture() override { running_ = false; }

    std::optional<RawPacket> getNextPacket() override {
        if (!running_ || sent_ >= maxPkts_) {
            running_ = false;
            return std::nullopt;
        }
        RawPacket p;
        p.isMock            = true;
        p.simulatedSourceIp = "10.0.0." + std::to_string(sent_);
        p.simulatedDestIp   = "10.0.0.99";
        p.simulatedProtocol = 6;  // TCP
        p.simulatedSize     = 100;
        p.length            = 100;
        ++sent_;
        return p;
    }

    bool isRunning() const override { return running_; }

private:
    int  maxPkts_;
    int  maxIfaces_;
    int  selectedIface_ = -1;
    int  sent_          = 0;
    bool running_       = false;
};

// ---------------------------------------------------------------------------
// PacketCapture tests
// ---------------------------------------------------------------------------

// 1. listInterfaces() delegates to strategy and returns non-empty list
TEST(PacketCaptureTest, ListInterfacesReturnsNonEmpty) {
    PacketCapture cap(std::make_unique<FastTestStrategy>());
    auto ifaces = cap.listInterfaces();
    EXPECT_FALSE(ifaces.empty());
    EXPECT_EQ(ifaces.size(), 3u);
}

// 2. selectInterface() with valid index succeeds
TEST(PacketCaptureTest, SelectInterfaceValidIndex) {
    PacketCapture cap(std::make_unique<FastTestStrategy>());
    EXPECT_TRUE(cap.selectInterface(0));
    EXPECT_TRUE(cap.selectInterface(2));
}

// 3. selectInterface() with negative index fails
TEST(PacketCaptureTest, SelectInterfaceNegativeIndex) {
    PacketCapture cap(std::make_unique<FastTestStrategy>());
    EXPECT_FALSE(cap.selectInterface(-1));
}

// 4. selectInterface() with out-of-range index fails
TEST(PacketCaptureTest, SelectInterfaceOutOfRange) {
    PacketCapture cap(std::make_unique<FastTestStrategy>());
    EXPECT_FALSE(cap.selectInterface(10));
}

// 5. startCapture() without prior selectInterface fails
TEST(PacketCaptureTest, StartCaptureWithoutSelectFails) {
    PacketCapture cap(std::make_unique<FastTestStrategy>());
    EXPECT_FALSE(cap.startCapture());
    EXPECT_FALSE(cap.isRunning());
}

// 6. startCapture() after selectInterface succeeds and isRunning becomes true
TEST(PacketCaptureTest, StartCaptureAfterSelectSucceeds) {
    PacketCapture cap(std::make_unique<FastTestStrategy>());
    ASSERT_TRUE(cap.selectInterface(0));
    EXPECT_TRUE(cap.startCapture());
    EXPECT_TRUE(cap.isRunning());
}

// 7. getNextPacket() returns valid packets while running then nullopt
TEST(PacketCaptureTest, GetNextPacketDrainsAndStops) {
    PacketCapture cap(std::make_unique<FastTestStrategy>(3));
    ASSERT_TRUE(cap.selectInterface(0));
    ASSERT_TRUE(cap.startCapture());

    int count = 0;
    while (cap.isRunning()) {
        auto pkt = cap.getNextPacket();
        if (pkt.has_value()) ++count;
    }
    EXPECT_EQ(count, 3);
    EXPECT_FALSE(cap.isRunning());
}

// 8. stopCapture() sets isRunning() to false
TEST(PacketCaptureTest, StopCaptureStopsRunning) {
    PacketCapture cap(std::make_unique<FastTestStrategy>(100));
    ASSERT_TRUE(cap.selectInterface(1));
    ASSERT_TRUE(cap.startCapture());
    EXPECT_TRUE(cap.isRunning());
    cap.stopCapture();
    EXPECT_FALSE(cap.isRunning());
}

// 9. Constructor with null strategy throws
TEST(PacketCaptureTest, NullStrategyThrows) {
    EXPECT_THROW(PacketCapture cap(nullptr), std::invalid_argument);
}

// 10. MockCaptureStrategy integration: returns exactly 30 packets
TEST(MockCaptureStrategyTest, Delivers30Packets) {
    MockCaptureStrategy strategy;
    ASSERT_TRUE(strategy.selectInterface(0));
    ASSERT_TRUE(strategy.startCapture());

    int count = 0;
    while (strategy.isRunning()) {
        auto pkt = strategy.getNextPacket();
        if (pkt.has_value()) ++count;
    }
    EXPECT_EQ(count, 30);
}
