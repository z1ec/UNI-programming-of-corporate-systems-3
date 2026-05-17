#include "PacketCapture.h"
#include "PacketParser.h"
#include "TrafficStatistics.h"
#include "ReportGenerator.h"
#include "ConsoleUI.h"

#include <iostream>
#include <memory>

// ---------------------------------------------------------------------------
// main – application entry point.
//
// Construction order (Dependency Injection):
//   1. Factory creates the appropriate capture strategy (mock or real pcap).
//   2. Each subsystem is constructed independently.
//   3. ConsoleUI receives references to all subsystems (it owns nothing).
//   4. ConsoleUI.run() drives the entire application loop.
// ---------------------------------------------------------------------------
int main() {
    try {
        // ── Step 1: select capture strategy via factory ──────────────────
        auto strategy = createCaptureStrategy();

        // ── Step 2: build subsystems (Dependency Injection) ──────────────
        PacketCapture     capture(std::move(strategy));
        PacketParser      parser;
        TrafficStatistics stats;
        ReportGenerator   reporter("traffic_report.txt");

        // ── Step 3: wire the Facade ───────────────────────────────────────
        ConsoleUI ui(capture, parser, stats, reporter);

        // ── Step 4: run ───────────────────────────────────────────────────
        ui.run();

    } catch (const std::exception& ex) {
        std::cerr << "\n[FATAL] " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
