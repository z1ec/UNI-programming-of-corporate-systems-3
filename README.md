# NetworkPacketSniffer

A console-based **network packet traffic analyzer** written in C++17.  
Practical Assignment #4 – Build Systems & Testing.

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
├── CMakeLists.txt          # CMake build definition (≥ 3.14, C++17)
├── README.md               # This file
├── include/
│   ├── PacketInfo.h        # PacketInfo struct + Protocol enum
│   ├── PacketCapture.h     # ICaptureStrategy, MockCaptureStrategy, PacketCapture
│   ├── PacketParser.h      # PacketParser class
│   ├── PcapCaptureStrategy.h  # Real pcap backend (USE_PCAP builds only)
│   ├── TrafficStatistics.h # TrafficStatistics + IpStats
│   ├── ReportGenerator.h   # ReportGenerator class
│   └── ConsoleUI.h         # ConsoleUI class (Facade)
├── src/
│   ├── main.cpp            # Entry point – wires all subsystems
│   ├── PacketCapture.cpp   # Strategy implementations + factory
│   ├── PacketParser.cpp    # Parsing logic + protocolToString()
│   ├── PcapCaptureStrategy.cpp  # Real pcap backend
│   ├── TrafficStatistics.cpp
│   ├── ReportGenerator.cpp
│   └── ConsoleUI.cpp
└── tests/
    ├── CMakeLists.txt           # Test build rules (GoogleTest via FetchContent)
    ├── test_PacketParser.cpp    # 9 unit tests
    ├── test_PacketCapture.cpp   # 10 unit tests
    ├── test_PcapCaptureStrategy.cpp  # 9 interface/contract tests
    ├── test_TrafficStatistics.cpp    # 8 unit tests
    ├── test_ReportGenerator.cpp      # 7 unit tests
    ├── test_ConsoleUI.cpp            # 7 integration tests
    └── scenarios/
        ├── scenario_mock_capture.cpp     # End-to-end mock session
        ├── scenario_real_capture.cpp     # Real capture or factory fallback
        ├── scenario_export_report.cpp    # Capture → report pipeline
        └── scenario_invalid_interface.cpp # Error-handling paths
```

---

## Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.14 |
| C++ compiler | GCC ≥ 8, Clang ≥ 7, or MSVC 2019+ |
| Internet access (first build) | GoogleTest is fetched automatically |
| libpcap / Npcap | Optional (mock mode works without it) |

---

## Build Instructions

### Quick start (mock mode, no libpcap needed)

```bash
# Configure (out-of-source build)
cmake -S . -B build

# Compile everything – app + all tests
cmake --build build

# Run the application
./build/NetworkPacketSniffer        # Linux / macOS
.\build\NetworkPacketSniffer.exe    # Windows
```

### Build with real libpcap (optional)

```bash
# Install libpcap first:
#   Ubuntu/Debian : sudo apt install libpcap-dev
#   macOS (brew)  : brew install libpcap
#   Windows       : install Npcap SDK from https://npcap.com

cmake -S . -B build           # libpcap detected automatically
cmake --build build
sudo ./build/NetworkPacketSniffer    # root / Administrator required
```

---

## Running Tests

### Run all tests (unit tests + scenarios)

```bash
ctest --test-dir build
```

### Run with verbose output

```bash
ctest --test-dir build --verbose
```

### Run with detailed output on failure

```bash
ctest --test-dir build --output-on-failure
```

### Run a specific test suite

```bash
ctest --test-dir build -R PacketParser
ctest --test-dir build -R TrafficStatistics
ctest --test-dir build -R ConsoleUI
```

### Run a scenario manually

```bash
./build/tests/scenario_mock_capture
./build/tests/scenario_export_report
./build/tests/scenario_invalid_interface
./build/tests/scenario_real_capture   # needs root + libpcap
```

### Run unit tests directly (GoogleTest output)

```bash
./build/tests/test_PacketParser
./build/tests/test_TrafficStatistics
./build/tests/test_ReportGenerator
# etc.
```

---

## Test Coverage

### Unit tests (`tests/test_*.cpp`)

| File | Class | Tests |
|------|-------|-------|
| `test_PacketParser.cpp` | `PacketParser` | 9 |
| `test_PacketCapture.cpp` | `PacketCapture`, `MockCaptureStrategy` | 10 |
| `test_PcapCaptureStrategy.cpp` | `ICaptureStrategy` contract, factory | 9 |
| `test_TrafficStatistics.cpp` | `TrafficStatistics` | 8 |
| `test_ReportGenerator.cpp` | `ReportGenerator` | 7 |
| `test_ConsoleUI.cpp` | `ConsoleUI` | 7 |

Each test class covers: valid input, invalid input, edge cases, empty data,
unsupported protocols, and incorrect/malformed packets.

### Scenario programs (`tests/scenarios/`)

| Scenario | What it tests |
|----------|--------------|
| `scenario_mock_capture` | Interface listing & selection, start/stop, TCP/UDP/ICMP parsing, statistics |
| `scenario_real_capture` | Real pcap path (if available) or factory fallback |
| `scenario_export_report` | Full capture-to-report pipeline, file content verification |
| `scenario_invalid_interface` | Error handling: invalid index, null strategy, empty packets |

---

## Implemented SRS Requirements

| # | Requirement | Status |
|---|-------------|--------|
| 1 | Show available network interfaces | ✅ `PacketCapture::listInterfaces()` |
| 2 | Allow the user to select an interface | ✅ `PacketCapture::selectInterface()` |
| 3 | Start packet capture | ✅ `PacketCapture::startCapture()` |
| 4 | Stop packet capture | ✅ `PacketCapture::stopCapture()` |
| 5 | Analyze IPv4/IPv6, TCP, UDP, ICMP packets | ✅ `PacketParser::parse()` |
| 6 | Extract source and destination IP addresses | ✅ `PacketInfo::sourceIp`, `destinationIp` |
| 7 | Count packets by IP address | ✅ `TrafficStatistics::getIpStats()` |
| 8 | Count packets by protocol | ✅ `TrafficStatistics::getProtocolStats()` |
| 9 | Calculate total traffic volume in bytes | ✅ `TrafficStatistics::getTotalBytes()` |
| 10 | Display statistics in the console | ✅ `ConsoleUI::handleShowStatistics()` |
| 11 | Generate a final report | ✅ `ReportGenerator::generateReport()` |
| 12 | Handle missing permissions / interface / driver | ✅ Error checks throughout |
| 13 | Modular architecture | ✅ Five independent modules |
| 14 | GoogleTest unit tests | ✅ 50+ tests across 6 test files |
| 15 | Scenario integration programs | ✅ 4 scenario executables |
| 16 | CTest integration | ✅ `ctest --test-dir build` |

---

## Design Patterns

### 1. Facade — `ConsoleUI`

`ConsoleUI` exposes a single `run()` method that hides the complexity of four
subsystems.

### 2. Strategy — `ICaptureStrategy`

`MockCaptureStrategy` (built-in) and `PcapCaptureStrategy` (libpcap) share the
same interface.  Swapping backends requires no change to any other class.

### 3. Factory Method — `createCaptureStrategy()`

Returns `MockCaptureStrategy` or `PcapCaptureStrategy` based on the build
configuration (`USE_PCAP` define).

### 4. Dependency Injection

All subsystems are constructed in `main()` and injected into `ConsoleUI` by
reference, making every component independently testable.

---

## Third-Party Libraries

| Library | Purpose | Required? |
|---------|---------|-----------|
| GoogleTest (v1.14.0) | Unit testing framework (auto-fetched) | For tests only |
| libpcap / Npcap | Real packet capture | No (mock mode is the default) |
| C++17 STL | `std::optional`, smart pointers, `<chrono>`, … | Yes |
