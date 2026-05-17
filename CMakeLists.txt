cmake_minimum_required(VERSION 3.14)

# ---------------------------------------------------------------------------
# Project declaration
# ---------------------------------------------------------------------------
project(NetworkPacketSniffer
    VERSION     1.0.0
    DESCRIPTION "Console-based network packet traffic analyzer (Practical Assignment #4)"
    LANGUAGES   CXX
)

# ---------------------------------------------------------------------------
# Language standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ---------------------------------------------------------------------------
# Source lists (explicit – no GLOB)
# ---------------------------------------------------------------------------
set(LIB_SOURCES
    src/PacketCapture.cpp
    src/PacketParser.cpp
    src/PcapCaptureStrategy.cpp
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
# Shared library target  (used by the app executable AND the test targets)
# ---------------------------------------------------------------------------
add_library(sniffer_lib STATIC ${LIB_SOURCES} ${HEADERS})

target_include_directories(sniffer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(sniffer_lib PRIVATE /W4)
else()
    target_compile_options(sniffer_lib PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    )
endif()

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sniffer_lib)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    )
endif()

# ---------------------------------------------------------------------------
# Optional: libpcap / Npcap detection
# ---------------------------------------------------------------------------
find_library(PCAP_LIBRARY
    NAMES pcap wpcap
    PATHS /usr/lib /usr/local/lib /opt/homebrew/lib
)
find_path(PCAP_INCLUDE_DIR
    NAMES pcap.h pcap/pcap.h
    PATHS /usr/include /usr/local/include /opt/homebrew/include
)

if(PCAP_LIBRARY AND PCAP_INCLUDE_DIR)
    message(STATUS "[NetworkPacketSniffer] libpcap found at ${PCAP_LIBRARY}")
    message(STATUS "[NetworkPacketSniffer] Enabling REAL packet capture mode (USE_PCAP)")

    target_compile_definitions(sniffer_lib PUBLIC USE_PCAP=1)
    target_include_directories(sniffer_lib PUBLIC ${PCAP_INCLUDE_DIR})
    target_link_libraries(sniffer_lib PUBLIC ${PCAP_LIBRARY})

    if(UNIX AND NOT APPLE)
        find_package(Threads REQUIRED)
        target_link_libraries(sniffer_lib PUBLIC Threads::Threads)
    endif()

    if(WIN32)
        target_link_libraries(sniffer_lib PUBLIC ws2_32)
    endif()
else()
    message(STATUS "[NetworkPacketSniffer] libpcap NOT found – using mock/simulation mode")
endif()

# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------
include(CTest)
enable_testing()
add_subdirectory(tests)

# ---------------------------------------------------------------------------
# Install rule
# ---------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

message(STATUS "")
message(STATUS "  Build with:       cmake --build ${CMAKE_BINARY_DIR}")
message(STATUS "  Run with:         ${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
message(STATUS "  Run tests with:   ctest --test-dir ${CMAKE_BINARY_DIR}")
message(STATUS "")
