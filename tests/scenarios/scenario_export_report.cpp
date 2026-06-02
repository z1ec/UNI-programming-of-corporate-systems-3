

#include "PacketCapture.h"
#include "PacketParser.h"
#include "TrafficStatistics.h"
#include "ReportGenerator.h"
#include "PacketInfo.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

int main() {
    std::cout << "[scenario_export_report] Starting...\n";


    PacketCapture capture(std::make_unique<MockCaptureStrategy>());

    if (!capture.selectInterface(0)) {
        std::cerr << "[FAIL] selectInterface.\n";
        return 1;
    }
    if (!capture.startCapture()) {
        std::cerr << "[FAIL] startCapture.\n";
        return 1;
    }

    PacketParser      parser;
    TrafficStatistics stats;

    while (capture.isRunning()) {
        auto raw = capture.getNextPacket();
        if (!raw.has_value()) continue;
        stats.addPacket(parser.parse(*raw));
    }

    if (stats.getTotalPackets() == 0) {
        std::cerr << "[FAIL] No packets processed.\n";
        return 1;
    }
    std::cout << "[OK] Captured " << stats.getTotalPackets() << " packets, "
              << stats.getTotalBytes() << " bytes.\n";


    const std::string tmpDir  = (fs::temp_directory_path() / "sniffer_tests").string();
    const std::string outPath = tmpDir + "/scenario_report.txt";
    fs::create_directories(tmpDir);

    ReportGenerator reporter(outPath);
    if (!reporter.generateReport(stats)) {
        std::cerr << "[FAIL] generateReport returned false.\n";
        return 1;
    }
    if (!fs::exists(outPath)) {
        std::cerr << "[FAIL] Report file does not exist at: " << outPath << "\n";
        return 1;
    }
    std::cout << "[OK] Report written to: " << outPath << "\n";


    const std::string content = readFile(outPath);

    if (!contains(content, "TRAFFIC REPORT")) {
        std::cerr << "[FAIL] Report missing header.\n";
        return 1;
    }
    if (!contains(content, "GLOBAL TOTALS")) {
        std::cerr << "[FAIL] Report missing global totals section.\n";
        return 1;
    }
    if (!contains(content, "TCP")) {
        std::cerr << "[FAIL] Report missing TCP entry.\n";
        return 1;
    }
    if (!contains(content, "UDP")) {
        std::cerr << "[FAIL] Report missing UDP entry.\n";
        return 1;
    }
    if (!contains(content, "ICMP")) {
        std::cerr << "[FAIL] Report missing ICMP entry.\n";
        return 1;
    }
    if (!contains(content, "192.168.")) {
        std::cerr << "[FAIL] Report missing expected source IPs.\n";
        return 1;
    }
    std::cout << "[OK] Report content verified (TCP/UDP/ICMP and IPs present).\n";


    ReportGenerator bad("/no_such_dir/x/y/report.txt");
    if (bad.generateReport(stats)) {
        std::cerr << "[FAIL] generateReport to invalid path should return false.\n";
        return 1;
    }
    std::cout << "[OK] Invalid path correctly rejected.\n";

    std::cout << "[scenario_export_report] PASSED\n";
    return 0;
}
