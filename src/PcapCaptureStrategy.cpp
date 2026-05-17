#include "PcapCaptureStrategy.h"

// The entire translation unit is compiled only when libpcap is available.
#ifdef USE_PCAP

#include <pcap.h>
#include <cstring>
#include <iostream>

// ============================================================================
//  PcapCaptureStrategy
// ============================================================================

PcapCaptureStrategy::PcapCaptureStrategy(int maxPackets)
    : maxPackets_(maxPackets)
{
    std::memset(errbuf_, 0, PCAP_ERRBUF_SIZE);
}

PcapCaptureStrategy::~PcapCaptureStrategy() {
    // RAII: always release the pcap handle on destruction.
    if (handle_) {
        pcap_close(handle_);
        handle_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// refreshDeviceNames – internal helper
// Runs pcap_findalldevs and populates deviceNames_ only (no display strings).
// Called by both listInterfaces() and selectInterface() so neither needs to
// call the other and trigger the [[nodiscard]] warning.
// ---------------------------------------------------------------------------
void PcapCaptureStrategy::refreshDeviceNames() {
    pcap_if_t* alldevs = nullptr;
    char       errbuf[PCAP_ERRBUF_SIZE];

    deviceNames_.clear();

    if (pcap_findalldevs(&alldevs, errbuf) == PCAP_ERROR) {
        lastError_ = errbuf;
        lastError_ += " (try running as root / Administrator)";
        return;
    }

    for (const pcap_if_t* dev = alldevs; dev != nullptr; dev = dev->next) {
        deviceNames_.push_back(dev->name);
    }

    pcap_freealldevs(alldevs);
}

// ---------------------------------------------------------------------------
// listInterfaces
// ---------------------------------------------------------------------------
std::vector<std::string> PcapCaptureStrategy::listInterfaces() {
    pcap_if_t* alldevs = nullptr;
    char       errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&alldevs, errbuf) == PCAP_ERROR) {
        lastError_ = errbuf;
        lastError_ += " (try running as root / Administrator)";
        return {};
    }

    std::vector<std::string> display;
    deviceNames_.clear();

    for (const pcap_if_t* dev = alldevs; dev != nullptr; dev = dev->next) {
        deviceNames_.push_back(dev->name);

        std::string entry = dev->name;
        if (dev->description && dev->description[0] != '\0') {
            entry += "  (";
            entry += dev->description;
            entry += ')';
        }
        display.push_back(std::move(entry));
    }

    pcap_freealldevs(alldevs);
    return display;
}

// ---------------------------------------------------------------------------
// selectInterface
// ---------------------------------------------------------------------------
bool PcapCaptureStrategy::selectInterface(int index) {
    // Populate deviceNames_ if the user hasn't called listInterfaces() yet.
    if (deviceNames_.empty()) {
        refreshDeviceNames();  // no [[nodiscard]] – void return
    }

    if (index < 0 || index >= static_cast<int>(deviceNames_.size())) {
        lastError_ = "Interface index out of range";
        return false;
    }

    selectedIface_ = index;
    return true;
}

// ---------------------------------------------------------------------------
// startCapture
// ---------------------------------------------------------------------------
bool PcapCaptureStrategy::startCapture() {
    if (selectedIface_ < 0 ||
        selectedIface_ >= static_cast<int>(deviceNames_.size())) {
        lastError_ = "No interface selected";
        return false;
    }

    // Open the interface in promiscuous mode.
    // Snapshot length 65535 – capture every byte of every frame.
    // Read timeout   100 ms  – pcap_next_ex returns 0 after 100 ms if no
    //                          packet has arrived, keeping the capture loop
    //                          responsive without busy-waiting.
    handle_ = pcap_open_live(
        deviceNames_[static_cast<std::size_t>(selectedIface_)].c_str(),
        65535,   // snaplen
        1,       // promiscuous
        100,     // read timeout (ms)
        errbuf_
    );

    if (!handle_) {
        lastError_ = errbuf_;
        if (lastError_.find("perm") != std::string::npos ||
            lastError_.find("root") != std::string::npos ||
            lastError_.find("admin") != std::string::npos) {
            lastError_ += " – try running as root / Administrator";
        }
        return false;
    }

    // Apply a BPF filter so only IPv4 and IPv6 frames reach the application.
    struct bpf_program fp{};
    const char* filter = "ip or ip6";
    // 0 = do not optimize; PCAP_NETMASK_UNKNOWN = don't care about netmask
    if (pcap_compile(handle_, &fp, filter, 0, PCAP_NETMASK_UNKNOWN) == 0) {
        pcap_setfilter(handle_, &fp);
        pcap_freecode(&fp);
    } else {
        // Non-fatal: continue without the filter.
        std::cerr << "[PcapCaptureStrategy] Warning: BPF compile failed: "
                  << pcap_geterr(handle_) << "\n";
    }

    running_         = true;
    packetsCaptured_ = 0;
    lastError_.clear();
    return true;
}

// ---------------------------------------------------------------------------
// stopCapture
// ---------------------------------------------------------------------------
void PcapCaptureStrategy::stopCapture() {
    running_ = false;
    if (handle_) {
        pcap_breakloop(handle_);  // unblock any pending pcap_next_ex call
        pcap_close(handle_);
        handle_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// getNextPacket
//
// Return semantics (same contract as MockCaptureStrategy):
//   has_value() == true   → valid packet; isRunning() unchanged
//   has_value() == false
//     isRunning() == true  → read timeout; caller should call again
//     isRunning() == false → error, breakloop, or packet limit reached;
//                            caller should exit the capture loop
// ---------------------------------------------------------------------------
std::optional<RawPacket> PcapCaptureStrategy::getNextPacket() {
    if (!running_ || !handle_) {
        return std::nullopt;
    }

    // Honour the configured per-session packet limit.
    if (packetsCaptured_ >= maxPackets_) {
        running_ = false;
        return std::nullopt;
    }

    struct pcap_pkthdr* header = nullptr;
    const u_char*       data   = nullptr;

    const int res = pcap_next_ex(handle_, &header, &data);

    if (res == 0) {
        // Read timeout – no packet available right now; keep running.
        return std::nullopt;
    }

    if (res == PCAP_ERROR_BREAK) {
        // pcap_breakloop() was called (e.g. from stopCapture()).
        running_ = false;
        return std::nullopt;
    }

    if (res == PCAP_ERROR) {
        lastError_ = pcap_geterr(handle_);
        running_   = false;
        return std::nullopt;
    }

    // res == 1: packet successfully read.
    ++packetsCaptured_;

    RawPacket pkt;
    pkt.isMock = false;
    pkt.length = header->caplen;
    pkt.data.assign(data, data + header->caplen);

    return pkt;
}

// ---------------------------------------------------------------------------
// isRunning / lastError
// ---------------------------------------------------------------------------
bool PcapCaptureStrategy::isRunning() const {
    return running_;
}

const std::string& PcapCaptureStrategy::lastError() const {
    return lastError_;
}

// ---------------------------------------------------------------------------
// Factory function (called from createCaptureStrategy in PacketCapture.cpp)
// ---------------------------------------------------------------------------
std::unique_ptr<ICaptureStrategy> createPcapStrategy() {
    return std::make_unique<PcapCaptureStrategy>(50);
}

#endif // USE_PCAP
