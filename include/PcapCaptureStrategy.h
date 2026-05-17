#pragma once

// This header and its companion .cpp are only compiled when libpcap is present.
// The entire class definition is gated so that including this header in a
// non-pcap build produces an empty translation unit with no errors.
#ifdef USE_PCAP

#include "PacketCapture.h"

#include <pcap.h>
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// PcapCaptureStrategy – real capture backend (Strategy pattern, concrete).
//
// Uses libpcap/Npcap APIs:
//   pcap_findalldevs  – list interfaces
//   pcap_open_live    – open a handle in promiscuous mode
//   pcap_compile /
//   pcap_setfilter    – restrict capture to IP traffic only
//   pcap_next_ex      – retrieve one packet (non-blocking with 100 ms timeout)
//   pcap_breakloop /
//   pcap_close        – graceful shutdown
//
// The strategy captures up to maxPackets packets then signals completion by
// returning nullopt and setting isRunning() = false.  This keeps the UI loop
// identical to the mock flow.
//
// Requires root / Administrator privileges at runtime.
// ---------------------------------------------------------------------------
class PcapCaptureStrategy final : public ICaptureStrategy {
public:
    explicit PcapCaptureStrategy(int maxPackets = 50);
    ~PcapCaptureStrategy() override;

    // Non-copyable (owns a pcap_t* handle).
    PcapCaptureStrategy(const PcapCaptureStrategy&)            = delete;
    PcapCaptureStrategy& operator=(const PcapCaptureStrategy&) = delete;

    // ICaptureStrategy interface ─────────────────────────────────────────────
    [[nodiscard]] std::vector<std::string> listInterfaces()       override;
    bool                                   selectInterface(int i) override;
    bool                                   startCapture()         override;
    void                                   stopCapture()          override;
    [[nodiscard]] std::optional<RawPacket> getNextPacket()        override;
    [[nodiscard]] bool                     isRunning() const      override;

    // Returns the last libpcap error string (empty when no error occurred).
    [[nodiscard]] const std::string& lastError() const;

private:
    // Populate deviceNames_ from pcap_findalldevs without building display strings.
    // Used internally by listInterfaces() and selectInterface().
    void refreshDeviceNames();

    pcap_t*                  handle_          = nullptr;
    bool                     running_         = false;
    int                      selectedIface_   = -1;
    int                      maxPackets_;
    int                      packetsCaptured_ = 0;
    std::vector<std::string> deviceNames_;   // internal pcap device names
    std::string              lastError_;
    char                     errbuf_[PCAP_ERRBUF_SIZE]{};
};

// Factory function called from createCaptureStrategy() in PacketCapture.cpp.
std::unique_ptr<ICaptureStrategy> createPcapStrategy();

#endif // USE_PCAP
