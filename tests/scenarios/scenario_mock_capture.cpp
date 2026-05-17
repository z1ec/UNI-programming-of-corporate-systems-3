// scenario_mock_capture.cpp
//
// Scenario: run a complete mock capture session end-to-end.
//
// Covers:
//   • interface listing and selection
//   • start / stop capture
//   • TCP/UDP/ICMP packet parsing
//   • statistics calculation
//
// Returns 0 on success, 1 on any unexpected failure.

#include "PacketCapture.h"
#include "PacketParser.h"
#include "TrafficStatistics.h"
#include "PacketInfo.h"

#include <iostream>
#include <cassert>

int main() {
    std::cout << "[scenario_mock_capture] Starting...\n";

    // ── 1. Interface listing ───────────────────────────────────────────────
    PacketCapture capture(std::make_unique<MockCaptureStrategy>());
    auto ifaces = capture.listInterfaces();

    if (ifaces.empty()) {
        std::cerr << "[FAIL] No interfaces returned.\n";
        return 1;
    }
    std::cout << "[OK] Found " << ifaces.size() << " simulated interface(s).\n";

    // ── 2. Interface selection ─────────────────────────────────────────────
    if (!capture.selectInterface(0)) {
        std::cerr << "[FAIL] selectInterface(0) failed.\n";
        return 1;
    }
    std::cout << "[OK] Interface 0 selected.\n";

    // Invalid index must be rejected.
    if (capture.selectInterface(-1)) {
        std::cerr << "[FAIL] selectInterface(-1) should have failed.\n";
        return 1;
    }
    std::cout << "[OK] Negative index correctly rejected.\n";

    // ── 3. Start capture ───────────────────────────────────────────────────
    if (!capture.startCapture()) {
        std::cerr << "[FAIL] startCapture() failed.\n";
        return 1;
    }
    if (!capture.isRunning()) {
        std::cerr << "[FAIL] isRunning() should be true after start.\n";
        return 1;
    }
    std::cout << "[OK] Capture started.\n";

    // ── 4. Packet parsing & statistics ────────────────────────────────────
    PacketParser      parser;
    TrafficStatistics stats;
    int               packetCount = 0;

    while (capture.isRunning()) {
        auto raw = capture.getNextPacket();
        if (!raw.has_value()) continue;

        PacketInfo info = parser.parse(*raw);
        stats.addPacket(info);
        ++packetCount;

        // Each packet must have non-empty IPs and a known protocol.
        if (info.sourceIp.empty() || info.destinationIp.empty()) {
            std::cerr << "[FAIL] Packet " << packetCount << " has empty IP.\n";
            return 1;
        }
    }

    std::cout << "[OK] Processed " << packetCount << " packets.\n";

    if (packetCount == 0) {
        std::cerr << "[FAIL] No packets were processed.\n";
        return 1;
    }

    // ── 5. Statistics sanity checks ───────────────────────────────────────
    if (stats.getTotalPackets() != static_cast<std::size_t>(packetCount)) {
        std::cerr << "[FAIL] Statistics packet count mismatch.\n";
        return 1;
    }
    if (stats.getTotalBytes() == 0) {
        std::cerr << "[FAIL] Total bytes must be > 0.\n";
        return 1;
    }
    if (stats.getIpStats().empty()) {
        std::cerr << "[FAIL] IP stats must not be empty.\n";
        return 1;
    }
    if (stats.getProtocolStats().empty()) {
        std::cerr << "[FAIL] Protocol stats must not be empty.\n";
        return 1;
    }

    // TCP, UDP and ICMP packets must all appear.
    const auto& proto = stats.getProtocolStats();
    if (proto.count(Protocol::TCP) == 0 ||
        proto.count(Protocol::UDP) == 0 ||
        proto.count(Protocol::ICMP) == 0) {
        std::cerr << "[FAIL] Expected TCP, UDP, and ICMP in statistics.\n";
        return 1;
    }

    std::cout << "[OK] Statistics verified (TCP/UDP/ICMP all present).\n";

    // ── 6. Stop capture ───────────────────────────────────────────────────
    // After the mock drains, isRunning() is already false; stopCapture()
    // on a stopped capture must not crash.
    capture.stopCapture();
    if (capture.isRunning()) {
        std::cerr << "[FAIL] isRunning() should be false after stopCapture.\n";
        return 1;
    }
    std::cout << "[OK] Stop capture succeeded.\n";

    std::cout << "[scenario_mock_capture] PASSED\n";
    return 0;
}
