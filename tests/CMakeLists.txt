

cmake_minimum_required(VERSION 3.14)


include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)


set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)


set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)


macro(add_gtest target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE sniffer_lib GTest::gtest_main)
    target_include_directories(${target} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    if(NOT MSVC)
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
    include(GoogleTest)
    gtest_discover_tests(${target})
endmacro()


add_gtest(test_PacketParser       test_PacketParser.cpp)
add_gtest(test_PacketCapture      test_PacketCapture.cpp)
add_gtest(test_PcapCaptureStrategy test_PcapCaptureStrategy.cpp)
add_gtest(test_TrafficStatistics  test_TrafficStatistics.cpp)
add_gtest(test_ReportGenerator    test_ReportGenerator.cpp)
add_gtest(test_ConsoleUI          test_ConsoleUI.cpp)


macro(add_scenario target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE sniffer_lib)
    target_include_directories(${target} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    if(NOT MSVC)
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME ${target} COMMAND ${target})
endmacro()

add_scenario(scenario_mock_capture    scenarios/scenario_mock_capture.cpp)
add_scenario(scenario_real_capture    scenarios/scenario_real_capture.cpp)
add_scenario(scenario_export_report   scenarios/scenario_export_report.cpp)
add_scenario(scenario_invalid_interface scenarios/scenario_invalid_interface.cpp)


set_tests_properties(scenario_mock_capture PROPERTIES TIMEOUT 10)


set_tests_properties(scenario_real_capture PROPERTIES
    TIMEOUT 15
    WILL_FAIL FALSE
)
