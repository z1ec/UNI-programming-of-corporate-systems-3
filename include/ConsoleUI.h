#pragma once

#include "PacketCapture.h"
#include "PacketParser.h"
#include "TrafficStatistics.h"
#include "ReportGenerator.h"

// ---------------------------------------------------------------------------
// ConsoleUI – Facade that presents a simple text menu and coordinates all
// other subsystems (PacketCapture, PacketParser, TrafficStatistics,
// ReportGenerator).
//
// All four components are injected via the constructor so the UI itself has
// no knowledge of concrete strategies or file paths — it only orchestrates.
// ---------------------------------------------------------------------------
class ConsoleUI {
public:
    ConsoleUI(PacketCapture&    capture,
              PacketParser&     parser,
              TrafficStatistics& stats,
              ReportGenerator&  reporter);

    // Main application loop: shows the menu and dispatches choices.
    void run();

private:
    // Menu handlers ──────────────────────────────────────────────────────
    void handleListInterfaces();
    void handleSelectInterface();
    void handleStartCapture();
    void handleStopCapture();
    void handleShowStatistics();
    void handleExportReport();

    // Rendering helpers
    void printBanner()     const;
    void printMenu()       const;
    void printStats()      const;
    void printSeparator()  const;

    // Subsystems (references – lifetime managed by main())
    PacketCapture&     capture_;
    PacketParser&      parser_;
    TrafficStatistics& stats_;
    ReportGenerator&   reporter_;

    bool running_            = true;
    int  selectedInterface_  = -1;
};
