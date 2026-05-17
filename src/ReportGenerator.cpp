#include "ReportGenerator.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <iostream>

// ============================================================================
//  ReportGenerator
// ============================================================================

ReportGenerator::ReportGenerator(std::string outputPath)
    : outputPath_(std::move(outputPath))
{}

const std::string& ReportGenerator::getOutputPath() const {
    return outputPath_;
}

// ---------------------------------------------------------------------------
// buildReportText  – pure text construction, no file I/O.
// ---------------------------------------------------------------------------
std::string ReportGenerator::buildReportText(const TrafficStatistics& stats) const {
    std::ostringstream oss;

    // Timestamp
    const auto now   = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    char timeBuf[64] = {};
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    const std::string sep(60, '=');

    oss << sep << "\n"
        << "  NETWORK PACKET SNIFFER – TRAFFIC REPORT\n"
        << sep << "\n"
        << "Generated : " << timeBuf << "\n\n";

    // ── Global totals ──────────────────────────────────────────────────────
    oss << "GLOBAL TOTALS\n"
        << std::string(30, '-') << "\n"
        << "  Total packets captured : " << stats.getTotalPackets() << "\n"
        << "  Total traffic volume   : " << stats.getTotalBytes() << " bytes"
        << "  (" << (stats.getTotalBytes() / 1024.0) << " KB)\n\n";

    // ── Protocol breakdown ─────────────────────────────────────────────────
    oss << "PACKETS BY PROTOCOL\n"
        << std::string(30, '-') << "\n";
    for (const auto& [proto, count] : stats.getProtocolStats()) {
        oss << "  " << std::left << std::setw(10) << protocolToString(proto)
            << " : " << count << " packets\n";
    }
    oss << "\n";

    // ── Per-IP breakdown ───────────────────────────────────────────────────
    oss << "PACKETS BY SOURCE IP ADDRESS\n"
        << std::string(30, '-') << "\n";
    for (const auto& [ip, s] : stats.getIpStats()) {
        oss << "  " << std::left << std::setw(18) << ip
            << "  packets=" << std::setw(5) << s.packetCount
            << "  bytes=" << s.totalBytes << "\n";
    }
    oss << "\n" << sep << "\n"
        << "  End of report\n"
        << sep << "\n";

    return oss.str();
}

// ---------------------------------------------------------------------------
// generateReport  – build text and write to file.
// ---------------------------------------------------------------------------
bool ReportGenerator::generateReport(const TrafficStatistics& stats) const {
    const std::string text = buildReportText(stats);

    std::ofstream file(outputPath_);
    if (!file.is_open()) {
        std::cerr << "[ReportGenerator] ERROR: cannot open '" << outputPath_
                  << "' for writing.\n";
        return false;
    }

    file << text;
    return file.good();
}
