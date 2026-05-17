cmake_minimum_required(VERSION 3.10)

# ---------------------------------------------------------------------------
# Project declaration
# ---------------------------------------------------------------------------
project(NetworkPacketSniffer
    VERSION     1.0.0
    DESCRIPTION "Console-based network packet traffic analyzer (Practical Assignment #3)"
    LANGUAGES   CXX
)

# ---------------------------------------------------------------------------
# Language standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)   # stick to portable ISO C++

# ---------------------------------------------------------------------------
# Source and header lists (explicit – no GLOB)
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/PacketCapture.cpp
    src/PacketParser.cpp
    src/PcapCaptureStrategy.cpp   # compiled to empty TU when USE_PCAP is off
    src/TrafficStatistics.cpp
    src/ReportGenerator.cpp
    src/ConsoleUI.cpp
)

set(HEADERS
    include/PacketInfo.h
    include/PacketCapture.h
    include/PacketParser.h
    include/PcapCaptureStrategy.h
    include/TrafficStatistics.h
    include/ReportGenerator.h
    include/ConsoleUI.h
)

# ---------------------------------------------------------------------------
# Executable target
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Expose the include/ directory to all files in this target.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ---------------------------------------------------------------------------
# Compiler warnings  (GCC/Clang and MSVC)
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    )
endif()

# ---------------------------------------------------------------------------
# Optional: libpcap / Npcap detection
#
# Search order:
#   1. find_package(PCAP) – uses a FindPCAP.cmake module if present
#   2. Manual find_library + find_path fallback (covers most distros)
#
# When found:
#   • PcapCaptureStrategy.cpp is compiled (its #ifdef USE_PCAP is satisfied)
#   • createPcapStrategy() in that file fulfils the forward declaration in
#     PacketCapture.cpp and the linker is happy
#
# When absent:
#   • PcapCaptureStrategy.cpp compiles to an empty translation unit
#   • createCaptureStrategy() returns MockCaptureStrategy instead
# ---------------------------------------------------------------------------
find_library(PCAP_LIBRARY
    NAMES pcap wpcap          # wpcap = Npcap / WinPcap on Windows
    PATHS /usr/lib /usr/local/lib /opt/homebrew/lib
)
find_path(PCAP_INCLUDE_DIR
    NAMES pcap.h pcap/pcap.h
    PATHS /usr/include /usr/local/include /opt/homebrew/include
)

if(PCAP_LIBRARY AND PCAP_INCLUDE_DIR)
    message(STATUS "[NetworkPacketSniffer] libpcap found at ${PCAP_LIBRARY}")
    message(STATUS "[NetworkPacketSniffer] Enabling REAL packet capture mode (USE_PCAP)")

    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_PCAP=1)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PCAP_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PCAP_LIBRARY})

    # On Linux, libpcap may depend on pthreads.
    if(UNIX AND NOT APPLE)
        find_package(Threads REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
    endif()

    # On Windows, link against Winsock (required by pcap headers).
    if(WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
    endif()
else()
    message(STATUS "[NetworkPacketSniffer] libpcap NOT found – using mock/simulation mode")
    message(STATUS "  Install libpcap-dev (Linux) or visit https://npcap.com (Windows)")
    message(STATUS "  then reconfigure with: cmake -S . -B build")
endif()

# ---------------------------------------------------------------------------
# Install rule (optional)
# ---------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# ---------------------------------------------------------------------------
# Quick-start usage (printed after configure):
#
#   cmake -S . -B build
#   cmake --build build
#   cd build && ./NetworkPacketSniffer
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "  Build with:  cmake --build ${CMAKE_BINARY_DIR}")
message(STATUS "  Run with:    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
message(STATUS "")
