#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>

// ---------------------------------------------------------------------------
// RawPacket – a packet as returned by the capture layer, before parsing.
//
// In real libpcap mode this would hold the raw byte buffer.  In mock/
// simulation mode the struct carries pre-filled logical fields instead.
// ---------------------------------------------------------------------------
struct RawPacket {
    std::vector<uint8_t> data;          // actual bytes (empty in mock mode)
    std::size_t          length = 0;    // on-wire length

    // Simulation fields populated by MockCaptureStrategy ──────────────────
    std::string simulatedSourceIp;
    std::string simulatedDestIp;
    int         simulatedProtocol = 0;  // IANA: 6=TCP, 17=UDP, 1=ICMP, 58=ICMPv6
    std::size_t simulatedSize     = 0;
    bool        isMock            = true;
};

// ---------------------------------------------------------------------------
// ICaptureStrategy – Strategy interface.
//
// Different concrete strategies implement the same contract:
//   • MockCaptureStrategy  – built-in simulation, no driver needed
//   • PcapCaptureStrategy  – real libpcap/Npcap (future addition)
//
// PacketCapture owns one strategy and delegates every call to it.
// ---------------------------------------------------------------------------
class ICaptureStrategy {
public:
    virtual ~ICaptureStrategy() = default;

    [[nodiscard]] virtual std::vector<std::string> listInterfaces()      = 0;
    virtual bool                                   selectInterface(int i) = 0;
    virtual bool                                   startCapture()         = 0;
    virtual void                                   stopCapture()          = 0;
    [[nodiscard]] virtual std::optional<RawPacket> getNextPacket()        = 0;
    [[nodiscard]] virtual bool                     isRunning() const      = 0;
};

// ---------------------------------------------------------------------------
// MockCaptureStrategy – simulation strategy (Strategy pattern, concrete).
//
// Generates a deterministic stream of 30 fake packets so the application
// compiles and runs without any external library.  Real libpcap can be
// added later by implementing a new PcapCaptureStrategy alongside this one.
// ---------------------------------------------------------------------------
class MockCaptureStrategy : public ICaptureStrategy {
public:
    [[nodiscard]] std::vector<std::string> listInterfaces()       override;
    bool                                   selectInterface(int i) override;
    bool                                   startCapture()         override;
    void                                   stopCapture()          override;
    [[nodiscard]] std::optional<RawPacket> getNextPacket()        override;
    [[nodiscard]] bool                     isRunning() const      override;

private:
    bool running_            = false;
    int  selectedInterface_  = -1;
    int  packetIndex_        = 0;
    static constexpr int TOTAL_MOCK_PACKETS = 30;
};

// ---------------------------------------------------------------------------
// PacketCapture – Facade over an ICaptureStrategy.
//
// Consumers interact only with this class; the concrete strategy is injected
// via the constructor (Dependency Injection) and can be swapped without
// changing any calling code.
// ---------------------------------------------------------------------------
class PacketCapture {
public:
    explicit PacketCapture(std::unique_ptr<ICaptureStrategy> strategy);

    [[nodiscard]] std::vector<std::string> listInterfaces();
    bool                                   selectInterface(int index);
    bool                                   startCapture();
    void                                   stopCapture();
    [[nodiscard]] std::optional<RawPacket> getNextPacket();
    [[nodiscard]] bool                     isRunning() const;

private:
    std::unique_ptr<ICaptureStrategy> strategy_;
};

// ---------------------------------------------------------------------------
// createCaptureStrategy – Factory function.
//
// Returns MockCaptureStrategy when compiled without USE_PCAP.
// When real libpcap support is added, compile with -DUSE_PCAP and provide
// a PcapCaptureStrategy implementation; this factory will return it instead.
// ---------------------------------------------------------------------------
std::unique_ptr<ICaptureStrategy> createCaptureStrategy();
