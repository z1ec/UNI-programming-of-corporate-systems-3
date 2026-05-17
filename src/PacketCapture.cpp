#include "PacketCapture.h"

#include <stdexcept>
#include <thread>
#include <chrono>
#include <array>

// ============================================================================
//  Mock packet templates
//  Each entry encodes: { sourceIp, destIp, protocolNumber, sizeBytes }
//  Protocol numbers follow IANA: 1=ICMP, 6=TCP, 17=UDP, 58=ICMPv6
// ============================================================================
namespace {

struct MockTemplate {
    const char* src;
    const char* dst;
    int         proto;
    std::size_t size;
};

constexpr std::array<MockTemplate, 10> kTemplates = {{
    { "192.168.1.10",  "8.8.8.8",         6,  1500 },  // TCP  – DNS lookup
    { "192.168.1.10",  "8.8.4.4",        17,    64 },  // UDP  – small query
    { "192.168.1.20",  "1.1.1.1",         1,    84 },  // ICMP – ping request
    { "10.0.0.5",      "192.168.1.10",    6,  1024 },  // TCP  – inbound data
    { "192.168.1.30",  "224.0.0.251",    17,   256 },  // UDP  – mDNS multicast
    { "192.168.1.10",  "192.168.1.20",   1,    84 },   // ICMP – local ping
    { "8.8.8.8",       "192.168.1.10",   6,  1500 },   // TCP  – HTTP response
    { "192.168.1.20",  "8.8.8.8",        17,  128 },   // UDP  – NTP update
    { "192.168.1.10",  "10.0.0.5",       6,   512 },   // TCP  – outbound data
    { "10.0.0.5",      "8.8.8.8",        1,    84 },   // ICMP – gateway ping
}};

} // anonymous namespace

// ============================================================================
//  MockCaptureStrategy
// ============================================================================

std::vector<std::string> MockCaptureStrategy::listInterfaces() {
    // Simulate common interface names.
    return {
        "eth0  (simulated – Ethernet)",
        "wlan0 (simulated – Wi-Fi)",
        "lo    (simulated – Loopback)"
    };
}

bool MockCaptureStrategy::selectInterface(int index) {
    if (index < 0 || index > 2) {
        return false;
    }
    selectedInterface_ = index;
    return true;
}

bool MockCaptureStrategy::startCapture() {
    if (selectedInterface_ < 0) {
        return false;  // no interface chosen yet
    }
    running_      = true;
    packetIndex_  = 0;
    return true;
}

void MockCaptureStrategy::stopCapture() {
    running_ = false;
}

std::optional<RawPacket> MockCaptureStrategy::getNextPacket() {
    if (!running_ || packetIndex_ >= TOTAL_MOCK_PACKETS) {
        running_ = false;
        return std::nullopt;
    }

    // Cycle through the template array deterministically.
    const auto& tmpl = kTemplates[packetIndex_ % kTemplates.size()];

    RawPacket pkt;
    pkt.isMock              = true;
    pkt.simulatedSourceIp   = tmpl.src;
    pkt.simulatedDestIp     = tmpl.dst;
    pkt.simulatedProtocol   = tmpl.proto;
    pkt.simulatedSize       = tmpl.size;
    pkt.length              = tmpl.size;

    ++packetIndex_;

    // Small delay so the capture feels live (50 ms per packet).
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    return pkt;
}

bool MockCaptureStrategy::isRunning() const {
    return running_;
}

// ============================================================================
//  PacketCapture  (delegates every call to the injected strategy)
// ============================================================================

PacketCapture::PacketCapture(std::unique_ptr<ICaptureStrategy> strategy)
    : strategy_(std::move(strategy))
{
    if (!strategy_) {
        throw std::invalid_argument("PacketCapture: strategy must not be null");
    }
}

std::vector<std::string> PacketCapture::listInterfaces() {
    return strategy_->listInterfaces();
}

bool PacketCapture::selectInterface(int index) {
    return strategy_->selectInterface(index);
}

bool PacketCapture::startCapture() {
    return strategy_->startCapture();
}

void PacketCapture::stopCapture() {
    strategy_->stopCapture();
}

std::optional<RawPacket> PacketCapture::getNextPacket() {
    return strategy_->getNextPacket();
}

bool PacketCapture::isRunning() const {
    return strategy_->isRunning();
}

// ============================================================================
//  Factory function
//  Compile with -DUSE_PCAP to swap in a real PcapCaptureStrategy.
// ============================================================================

#ifdef USE_PCAP
// Forward-declared in a future PcapCaptureStrategy.cpp
std::unique_ptr<ICaptureStrategy> createPcapStrategy();
#endif

std::unique_ptr<ICaptureStrategy> createCaptureStrategy() {
#ifdef USE_PCAP
    return createPcapStrategy();
#else
    return std::make_unique<MockCaptureStrategy>();
#endif
}
