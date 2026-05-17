# NetworkPacketSniffer

A console-based **network packet traffic analyzer** written in C++17.  
Practical Assignment #3 – Build Systems.

---

## Short Description

NetworkPacketSniffer captures (or simulates) network packets, parses IPv4/IPv6,
TCP, UDP, and ICMP headers, accumulates live statistics, and produces a final
text report.  When compiled without libpcap the application runs in a built-in
**mock/simulation mode** — no external driver or elevated privileges are needed.

---

## Project Structure

```
project-root/
├── CMakeLists.txt          # CMake build definition (≥ 3.10, C++17)
├── README.md               # This file
├── include/
│   ├── PacketInfo.h        # PacketInfo struct + Protocol enum
│   ├── PacketCapture.h     # ICaptureStrategy, MockCaptureStrategy, PacketCapture
│   ├── PacketParser.h      # PacketParser class
│   ├── TrafficStatistics.h # TrafficStatistics + IpStats
│   ├── ReportGenerator.h   # ReportGenerator class
│   └── ConsoleUI.h         # ConsoleUI class (Facade)
└── src/
    ├── main.cpp            # Entry point – wires all subsystems
    ├── PacketCapture.cpp   # Strategy implementations + factory
    ├── PacketParser.cpp    # Parsing logic + protocolToString()
    ├── TrafficStatistics.cpp
    ├── ReportGenerator.cpp
    └── ConsoleUI.cpp
```

---

## Implemented SRS Requirements

| # | Requirement | Status |
|---|-------------|--------|
| 1 | Show available network interfaces | ✅ `PacketCapture::listInterfaces()` |
| 2 | Allow the user to select an interface | ✅ `PacketCapture::selectInterface()` |
| 3 | Start packet capture | ✅ `PacketCapture::startCapture()` |
| 4 | Stop packet capture | ✅ `PacketCapture::stopCapture()` |
| 5 | Analyze IPv4/IPv6, TCP, UDP, ICMP packets | ✅ `PacketParser::parse()` / `mapProtocol()` |
| 6 | Extract source and destination IP addresses | ✅ `PacketInfo::sourceIp`, `destinationIp` |
| 7 | Count packets by IP address | ✅ `TrafficStatistics::getIpStats()` |
| 8 | Count packets by protocol | ✅ `TrafficStatistics::getProtocolStats()` |
| 9 | Calculate total traffic volume in bytes | ✅ `TrafficStatistics::getTotalBytes()` |
| 10 | Display statistics in the console | ✅ `ConsoleUI::handleShowStatistics()` |
| 11 | Generate a short final report | ✅ `ReportGenerator::generateReport()` → `traffic_report.txt` |
| 12 | Handle missing permissions / missing interface / no driver | ✅ Error checks in `startCapture()`, `selectInterface()`, factory |
| 13 | Modular architecture | ✅ Five independent modules (see structure above) |

---

## Design Patterns

### 1. Facade — `ConsoleUI`

`ConsoleUI` exposes a single `run()` method that hides the complexity of four
subsystems (`PacketCapture`, `PacketParser`, `TrafficStatistics`,
`ReportGenerator`).  Client code in `main.cpp` interacts only with the UI;
it never calls parser or statistics methods directly.

```cpp
// main.cpp – client sees only the Facade
ConsoleUI ui(capture, parser, stats, reporter);
ui.run();
```

### 2. Strategy — `ICaptureStrategy`

The abstract interface `ICaptureStrategy` defines the contract that every
capture backend must fulfil.  `MockCaptureStrategy` is the built-in
implementation; `PcapCaptureStrategy` (future) adds real libpcap support.
Swapping backends requires no change to any other class.

```cpp
class ICaptureStrategy {
public:
    virtual std::vector<std::string>   listInterfaces()       = 0;
    virtual bool                       selectInterface(int i) = 0;
    virtual bool                       startCapture()         = 0;
    virtual void                       stopCapture()          = 0;
    virtual std::optional<RawPacket>   getNextPacket()        = 0;
    virtual bool                       isRunning() const      = 0;
};
```

`PacketCapture` owns one strategy and delegates every call to it — the classic
Strategy pattern.

### 3. Factory Method — `createCaptureStrategy()`

A single factory function decides which strategy to instantiate based on build
configuration:

```cpp
// PacketCapture.cpp
std::unique_ptr<ICaptureStrategy> createCaptureStrategy() {
#ifdef USE_PCAP
    return createPcapStrategy();   // real capture (future)
#else
    return std::make_unique<MockCaptureStrategy>();
#endif
}
```

CMakeLists.txt defines `USE_PCAP` automatically when libpcap is found.  No
application code needs to change.

### 4. Dependency Injection (instead of Singleton)

All subsystems are constructed in `main()` and passed by reference to
`ConsoleUI`.  This makes every component independently testable — you can
inject a different `TrafficStatistics` instance or a different `ReportGenerator`
without touching the UI code.

```cpp
// main.cpp
PacketCapture     capture(std::move(strategy));
PacketParser      parser;
TrafficStatistics stats;
ReportGenerator   reporter("traffic_report.txt");

ConsoleUI ui(capture, parser, stats, reporter);  // DI
```

Global state (`Singleton`) is intentionally avoided.

### 5. Observer (described for future version)

In a real-time multi-threaded version, `TrafficStatistics` could implement an
**Observer** interface and subscribe to `PacketCapture` events.  Each time
`PacketCapture` emits a `packetCaptured` notification, all registered observers
(statistics display, logging, alerting) would update automatically — without
the capture loop knowing about them.

```
PacketCapture  ──(notifies)──►  IPacketObserver
                                 ├── TrafficStatistics
                                 ├── LiveDisplay
                                 └── AlertEngine
```

---

## Build Instructions

### Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.10 |
| C++ compiler | GCC ≥ 8, Clang ≥ 7, or MSVC 2019+ |
| libpcap / Npcap | Optional (mock mode works without it) |

### Build steps

```bash
# 1. Clone / navigate to the project root
cd NetworkPacketSniffer

# 2. Configure (out-of-source build)
cmake -S . -B build

# 3. Compile
cmake --build build

# 4. Run
cd build
./NetworkPacketSniffer        # Linux / macOS
.\NetworkPacketSniffer.exe    # Windows
```

To compile with real libpcap (once available):

```bash
cmake -S . -B build           # libpcap detected automatically
cmake --build build
sudo ./build/NetworkPacketSniffer    # root / Administrator required
```

---

## Run Instructions

After launching, a text menu appears:

```
MAIN MENU
  1. List available interfaces
  2. Select interface
  3. Start packet capture
  4. Stop packet capture
  5. Display current statistics
  6. Export final report
  0. Exit
```

Typical session:

1. Choose **1** to see available interfaces.
2. Choose **2** and enter an index to select one.
3. Choose **3** to start capture (30 simulated packets stream to the console).
4. Choose **5** to inspect live statistics.
5. Choose **6** to write `traffic_report.txt`.
6. Choose **0** to exit.

---

## Mock Mode and Future libpcap / Npcap Integration

When the project is built without libpcap the factory function returns a
`MockCaptureStrategy` that generates **30 deterministic fake packets** covering
TCP, UDP, and ICMP across several source and destination addresses.  The rest of
the pipeline (parser → statistics → report) is completely unchanged.

To add real capture later:

1. Implement `PcapCaptureStrategy : public ICaptureStrategy` in a new file
   (e.g. `src/PcapCaptureStrategy.cpp`).
2. Add a `createPcapStrategy()` factory function.
3. Compile with a libpcap-aware toolchain — CMake will detect the library and
   define `USE_PCAP` automatically.

No other source file needs to change.

---

## Third-Party Libraries

| Library | Purpose | Required? |
|---------|---------|-----------|
| libpcap / Npcap | Real packet capture | No (mock mode is the default) |
| C++17 STL | `std::optional`, smart pointers, `<chrono>`, `<map>`, … | Yes (provided by compiler) |

No external dependencies are required to build or run in mock mode.
