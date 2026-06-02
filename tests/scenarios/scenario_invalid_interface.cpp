

#include "PacketCapture.h"
#include "PacketParser.h"
#include "TrafficStatistics.h"
#include "PacketInfo.h"

#include <iostream>

int main() {
    std::cout << "[scenario_invalid_interface] Starting...\n";


    {
        PacketCapture cap(std::make_unique<MockCaptureStrategy>());
        if (cap.selectInterface(-1)) {
            std::cerr << "[FAIL] selectInterface(-1) should return false.\n";
            return 1;
        }
        std::cout << "[OK] Negative index rejected.\n";
    }


    {
        PacketCapture cap(std::make_unique<MockCaptureStrategy>());

        if (cap.selectInterface(99)) {
            std::cerr << "[FAIL] selectInterface(99) should return false.\n";
            return 1;
        }
        std::cout << "[OK] Out-of-range index rejected.\n";
    }


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


    {
        PacketCapture cap(std::make_unique<MockCaptureStrategy>());

        auto pkt = cap.getNextPacket();
        if (pkt.has_value()) {
            std::cerr << "[FAIL] getNextPacket should return nullopt when not running.\n";
            return 1;
        }
        std::cout << "[OK] getNextPacket returns nullopt when capture not started.\n";
    }


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


    {
        PacketCapture cap(std::make_unique<MockCaptureStrategy>());
        cap.stopCapture();  // must not crash
        if (cap.isRunning()) {
            std::cerr << "[FAIL] isRunning must stay false after stopCapture on idle.\n";
            return 1;
        }
        std::cout << "[OK] stopCapture on idle capture is a safe no-op.\n";
    }


    {
        PacketParser parser;
        RawPacket    empty;
        empty.isMock  = false;
        empty.length  = 0;


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
