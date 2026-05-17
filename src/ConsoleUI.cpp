#include "ConsoleUI.h"

#include <iostream>
#include <iomanip>
#include <limits>
#include <string>

// ============================================================================
//  ConsoleUI – Facade
//
//  All four subsystems (PacketCapture, PacketParser, TrafficStatistics,
//  ReportGenerator) are injected; ConsoleUI never constructs them directly.
// ============================================================================

ConsoleUI::ConsoleUI(PacketCapture&     capture,
                     PacketParser&      parser,
                     TrafficStatistics& stats,
                     ReportGenerator&   reporter)
    : capture_(capture)
    , parser_(parser)
    , stats_(stats)
    , reporter_(reporter)
{}

// ---------------------------------------------------------------------------
// run  – main application loop
// ---------------------------------------------------------------------------
void ConsoleUI::run() {
    printBanner();

    while (running_) {
        printMenu();

        int choice = -1;
        std::cout << "Your choice: ";
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  [!] Invalid input – please enter a number.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::cout << "\n";
        switch (choice) {
            case 1: handleListInterfaces();  break;
            case 2: handleSelectInterface(); break;
            case 3: handleStartCapture();    break;
            case 4: handleStopCapture();     break;
            case 5: handleShowStatistics();  break;
            case 6: handleExportReport();    break;
            case 0:
                std::cout << "  Exiting Network Packet Sniffer. Goodbye!\n";
                running_ = false;
                break;
            default:
                std::cout << "  [!] Unknown option. Please choose 0–6.\n";
                break;
        }
        std::cout << "\n";
    }
}

// ============================================================================
//  Menu handlers
// ============================================================================

void ConsoleUI::handleListInterfaces() {
    printSeparator();
    std::cout << "  AVAILABLE NETWORK INTERFACES\n";
    printSeparator();

    const auto interfaces = capture_.listInterfaces();
    if (interfaces.empty()) {
        std::cout << "  [!] No interfaces found.  "
                     "Check permissions or driver availability.\n";
        return;
    }

    for (int i = 0; i < static_cast<int>(interfaces.size()); ++i) {
        std::cout << "  [" << i << "] " << interfaces[i] << "\n";
    }
}

void ConsoleUI::handleSelectInterface() {
    printSeparator();
    std::cout << "  SELECT INTERFACE\n";
    printSeparator();

    handleListInterfaces();
    std::cout << "\n  Enter interface index: ";

    int idx = -1;
    if (!(std::cin >> idx)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  [!] Invalid input.\n";
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (capture_.selectInterface(idx)) {
        selectedInterface_ = idx;
        std::cout << "  [+] Interface " << idx << " selected.\n";
    } else {
        std::cout << "  [!] Invalid interface index.\n";
    }
}

void ConsoleUI::handleStartCapture() {
    printSeparator();
    std::cout << "  STARTING PACKET CAPTURE\n";
    printSeparator();

    if (selectedInterface_ < 0) {
        std::cout << "  [!] No interface selected. "
                     "Use option 2 first.\n";
        return;
    }

    if (capture_.isRunning()) {
        std::cout << "  [!] Capture is already running.\n";
        return;
    }

    stats_.reset();

    if (!capture_.startCapture()) {
        std::cout << "  [!] Failed to start capture. "
                     "Check permissions or driver availability.\n";
        return;
    }

#ifdef USE_PCAP
    std::cout << "  [+] Capture started (real mode – up to 50 live packets).\n"
              << "      Interrupt with Ctrl+C to stop early.\n\n";
#else
    std::cout << "  [+] Capture started (mock mode – 30 simulated packets).\n\n";
#endif
    std::cout << "  " << std::left
              << std::setw(18) << "Source IP"
              << std::setw(18) << "Destination IP"
              << std::setw(10) << "Protocol"
              << "Size\n";
    std::cout << "  " << std::string(54, '-') << "\n";

    // Drain packets from the capture strategy.
    // nullopt from getNextPacket() means either:
    //   • pcap read timeout  → isRunning() still true  → continue polling
    //   • mock finished      → isRunning() now false   → while condition fails
    //   • pcap error / limit → isRunning() now false   → while condition fails
    int count = 0;
    while (capture_.isRunning()) {
        auto raw = capture_.getNextPacket();
        if (!raw.has_value()) {
            continue;  // timeout; check isRunning() again on next iteration
        }

        const PacketInfo info = parser_.parse(*raw);
        stats_.addPacket(info);
        ++count;

        std::cout << "  " << std::left
                  << std::setw(18) << info.sourceIp
                  << std::setw(18) << info.destinationIp
                  << std::setw(10) << protocolToString(info.protocol)
                  << info.sizeBytes << " B\n";
    }

    std::cout << "\n  [+] Capture complete – " << count << " packets processed.\n";
}

void ConsoleUI::handleStopCapture() {
    printSeparator();
    std::cout << "  STOP CAPTURE\n";
    printSeparator();

    if (!capture_.isRunning()) {
        std::cout << "  [!] Capture is not currently running.\n";
        return;
    }
    capture_.stopCapture();
    std::cout << "  [+] Capture stopped.\n";
}

void ConsoleUI::handleShowStatistics() {
    printSeparator();
    std::cout << "  CURRENT STATISTICS\n";
    printSeparator();
    printStats();
}

void ConsoleUI::handleExportReport() {
    printSeparator();
    std::cout << "  EXPORT REPORT\n";
    printSeparator();

    if (stats_.getTotalPackets() == 0) {
        std::cout << "  [!] No packets captured yet. "
                     "Run a capture session first.\n";
        return;
    }

    if (reporter_.generateReport(stats_)) {
        std::cout << "  [+] Report saved to: " << reporter_.getOutputPath() << "\n";
    } else {
        std::cout << "  [!] Failed to write report file.\n";
    }
}

// ============================================================================
//  Rendering helpers
// ============================================================================

void ConsoleUI::printBanner() const {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║     Network Packet Sniffer / Traffic Analyzer        ║\n";
    std::cout << "  ║          Practical Assignment #3 – Build Systems     ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n";
#ifdef USE_PCAP
    std::cout << "  Running in REAL capture mode (libpcap).\n"
              << "  Note: packet capture requires root / Administrator privileges.\n\n";
#else
    std::cout << "  Running in MOCK/SIMULATION mode (no libpcap required).\n\n";
#endif
}

void ConsoleUI::printMenu() const {
    std::cout << "  ┌─────────────────────────────────┐\n";
    std::cout << "  │          MAIN MENU              │\n";
    std::cout << "  ├─────────────────────────────────┤\n";
    std::cout << "  │  1. List available interfaces   │\n";
    std::cout << "  │  2. Select interface            │\n";
    std::cout << "  │  3. Start packet capture        │\n";
    std::cout << "  │  4. Stop packet capture         │\n";
    std::cout << "  │  5. Display current statistics  │\n";
    std::cout << "  │  6. Export final report         │\n";
    std::cout << "  │  0. Exit                        │\n";
    std::cout << "  └─────────────────────────────────┘\n";
}

void ConsoleUI::printStats() const {
    const std::size_t total = stats_.getTotalPackets();
    if (total == 0) {
        std::cout << "  No data yet. Start a capture session first.\n";
        return;
    }

    std::cout << "\n  Total packets : " << total << "\n";
    std::cout << "  Total bytes   : " << stats_.getTotalBytes()
              << " (" << (stats_.getTotalBytes() / 1024.0) << " KB)\n\n";

    // Protocol breakdown
    std::cout << "  Protocol breakdown:\n";
    for (const auto& [proto, cnt] : stats_.getProtocolStats()) {
        const double pct = (total > 0)
            ? 100.0 * static_cast<double>(cnt) / static_cast<double>(total)
            : 0.0;
        std::cout << "    " << std::left << std::setw(10) << protocolToString(proto)
                  << std::right << std::setw(4) << cnt << " pkts"
                  << "  (" << std::fixed << std::setprecision(1) << pct << "%)\n";
    }

    // Per-IP breakdown
    std::cout << "\n  Top source addresses:\n";
    std::cout << "    " << std::left
              << std::setw(20) << "IP Address"
              << std::setw(10) << "Packets"
              << "Bytes\n";
    std::cout << "    " << std::string(40, '-') << "\n";
    for (const auto& [ip, s] : stats_.getIpStats()) {
        std::cout << "    " << std::left
                  << std::setw(20) << ip
                  << std::setw(10) << s.packetCount
                  << s.totalBytes << "\n";
    }
}

void ConsoleUI::printSeparator() const {
    std::cout << "  " << std::string(54, '-') << "\n";
}
