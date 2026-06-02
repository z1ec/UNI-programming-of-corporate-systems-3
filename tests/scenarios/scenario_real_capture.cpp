
#include "PacketCapture.h"
#include "PacketParser.h"
#include "TrafficStatistics.h"
#include "PacketInfo.h"

#include <iostream>
#include <string>

#ifdef USE_PCAP
#include "PcapCaptureStrategy.h"
#endif

int main() {
    std::cout << "[scenario_real_capture] Starting...\n";

#ifdef USE_PCAP
    std::cout << "[INFO] Built with libpcap – testing PcapCaptureStrategy.\n";

    PcapCaptureStrategy pcap(5); 


    auto ifaces = pcap.listInterfaces();
    if (ifaces.empty()) {
        std::cerr << "[WARN] No interfaces found: " << pcap.lastError() << "\n";
        std::cerr << "       This usually means the process lacks root/admin rights.\n";
        std::cout << "[scenario_real_capture] SKIPPED (insufficient privileges)\n";
        return 0;  
    }

    std::cout << "[OK] Found " << ifaces.size() << " real interface(s).\n";


    if (!pcap.selectInterface(0)) {
        std::cerr << "[WARN] Cannot select interface 0: " << pcap.lastError() << "\n";
        std::cout << "[scenario_real_capture] SKIPPED (interface select failed)\n";
        return 0;
    }
    std::cout << "[OK] Interface 0 selected.\n";

    if (!pcap.startCapture()) {
        std::cerr << "[WARN] startCapture failed: " << pcap.lastError() << "\n";
        std::cerr << "       Likely insufficient privileges (run as root/Administrator).\n";
        std::cout << "[scenario_real_capture] SKIPPED (needs root/Administrator)\n";
        return 0;
    }
    std::cout << "[OK] Real capture started (up to 5 packets).\n";

    PacketParser      parser;
    TrafficStatistics stats;
    int               count = 0;

    while (pcap.isRunning()) {
        auto raw = pcap.getNextPacket();
        if (!raw.has_value()) continue;

        PacketInfo info = parser.parse(*raw);
        stats.addPacket(info);
        ++count;
        std::cout << "  pkt " << count
                  << "  src=" << info.sourceIp
                  << "  dst=" << info.destinationIp
                  << "  proto=" << protocolToString(info.protocol)
                  << "  size=" << info.sizeBytes << " B\n";
    }

    pcap.stopCapture();
    std::cout << "[OK] Captured " << count << " real packet(s).\n";

#else

    std::cout << "[INFO] Built without libpcap – verifying factory fallback.\n";

    auto strategy = createCaptureStrategy();
    if (!strategy) {
        std::cerr << "[FAIL] createCaptureStrategy() returned null.\n";
        return 1;
    }

    auto ifaces = strategy->listInterfaces();
    if (ifaces.empty()) {
        std::cerr << "[FAIL] Factory strategy returned no interfaces.\n";
        return 1;
    }
    std::cout << "[OK] Factory returned a working mock strategy with "
              << ifaces.size() << " interface(s).\n";

    if (!strategy->selectInterface(0) || !strategy->startCapture()) {
        std::cerr << "[FAIL] Factory strategy could not start capture.\n";
        return 1;
    }


    int count = 0;
    while (strategy->isRunning() && count < 3) {
        auto pkt = strategy->getNextPacket();
        if (pkt.has_value()) ++count;
    }
    strategy->stopCapture();

    if (count == 0) {
        std::cerr << "[FAIL] Factory strategy produced no packets.\n";
        return 1;
    }
    std::cout << "[OK] Factory strategy delivered " << count << " packet(s).\n";
#endif

    std::cout << "[scenario_real_capture] PASSED\n";
    return 0;
}
