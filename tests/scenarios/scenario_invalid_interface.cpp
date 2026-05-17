// scenario_invalid_interface.cpp
//
// Scenario: verify correct error handling for invalid interface operations.
//
// Covers:
//   • selecting a non-existent interface (negative index, out-of-range)
//   • starting capture without selecting an interface first
//   • starting capture on an already-running session
//   • getNextPacket when not running returns nullopt

#include "PacketCapture.h"
#include "PacketParser.h"
#include "TrafficStatistics.h"
#include "PacketInfo.h"

#include <iostream>

int main() {
    std::cout << "[scenario_invalid_interface] Starting...\n";

    // ── 1. Negative interface index must be rejected ───────────────────────
    {
        PacketCapture cap(std::make_unique<MockCaptureStrategy>());
        if (cap.selectInterface(-1)) {
            std::cerr << "[FAIL] selectInterface(-1) should return false.\n";
            return 1;
        }
        std::cout << "[OK] Negative index rejected.\n";
    }

    // ── 2. Out-of-range index must be rejected ────────────────────────────
    {
        PacketCapture cap(std::make_unique<MockCaptureStrategy>());
        // MockCaptureStrategy exposes exactly 3 interfaces (indices 0-2).
        if (cap.selectInterface(99)) {
            std::cerr << "[FAIL] selectInterface(99) should return false.\n";
            return 1;
        }
        std::cout << "[OK] Out-of-range index rejected.\n";
    }

    // ── 3. startCapture without prior selectInterface must fail ───────────
    {
        PacketCapture cap(std::make_unique<MockCaptureStrategy>());
        if (cap.startCapture()) {
            std::cerr << "[FAIL] startCapture without select should return false.\n";
            return 1;
        }
        if (cap.isRunning()) {
            std::cerr << "[FAIL] isRunning must be false when startCapture failed.\n";
            return 1;
        }
        std::cout << "[OK] startCapture without selectInterface rejected.\n";
    }

    // ── 4. getNextPacket on a non-running capture returns nullopt ─────────
    {
        PacketCapture cap(std::make_unique<MockCaptureStrategy>());
        // Do NOT call selectInterface or startCapture.
        auto pkt = cap.getNextPacket();
        if (pkt.has_value()) {
            std::cerr << "[FAIL] getNextPacket should return nullopt when not running.\n";
            return 1;
        }
        std::cout << "[OK] getNextPacket returns nullopt when capture not started.\n";
    }

    // ── 5. Null strategy constructor throws std::invalid_argument ─────────
    {
        bool threw = false;
        try {
            PacketCapture cap(nullptr);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        if (!threw) {
            std::cerr << "[FAIL] Constructing PacketCapture with nullptr should throw.\n";
            return 1;
        }
        std::cout << "[OK] Null strategy throws std::invalid_argument.\n";
    }

    // ── 6. stopCapture on idle capture is a no-op (no crash) ─────────────
    {
        PacketCapture cap(std::make_unique<MockCaptureStrategy>());
        cap.stopCapture();  // must not crash
        if (cap.isRunning()) {
            std::cerr << "[FAIL] isRunning must stay false after stopCapture on idle.\n";
            return 1;
        }
        std::cout << "[OK] stopCapture on idle capture is a safe no-op.\n";
    }

    // ── 7. Parser on empty RawPacket (non-mock, no data) returns UNKNOWN ──
    {
        PacketParser parser;
        RawPacket    empty;
        empty.isMock  = false;
        empty.length  = 0;
        // data vector intentionally left empty

        PacketInfo info = parser.parse(empty);
        if (info.protocol != Protocol::UNKNOWN) {
            std::cerr << "[FAIL] Empty non-mock packet should parse to UNKNOWN.\n";
            return 1;
        }
        std::cout << "[OK] Empty real packet parsed as UNKNOWN.\n";
    }

    std::cout << "[scenario_invalid_interface] PASSED\n";
    return 0;
}
