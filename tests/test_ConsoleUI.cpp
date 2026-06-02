#include <gtest/gtest.h>
#include "ConsoleUI.h"
#include "PacketCapture.h"
#include "PacketParser.h"
#include "TrafficStatistics.h"
#include "ReportGenerator.h"

#include <sstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;


struct StdinRedirect {
    explicit StdinRedirect(const std::string& text)
        : buf_(text), old_(std::cin.rdbuf(buf_.rdbuf())) {}
    ~StdinRedirect() { std::cin.rdbuf(old_); }
    std::istringstream  buf_;
    std::streambuf*     old_;
};

struct StdoutCapture {
    StdoutCapture() : old_(std::cout.rdbuf(buf_.rdbuf())) {}
    ~StdoutCapture()            { std::cout.rdbuf(old_); }
    std::string str()     const { return buf_.str(); }
    std::ostringstream buf_;
    std::streambuf*    old_;
};


class ConsoleUITest : public ::testing::Test {
protected:
    void SetUp() override {
        fs::create_directories(kTmpDir_);
        strategy_ = std::make_unique<MockCaptureStrategy>();
        capture_  = std::make_unique<PacketCapture>(std::move(strategy_));
        reportPath_ = kTmpDir_ + "/ui_test_report.txt";
        reporter_   = std::make_unique<ReportGenerator>(reportPath_);
    }

    const std::string kTmpDir_ =
        (fs::temp_directory_path() / "sniffer_ui_tests").string();

    std::unique_ptr<MockCaptureStrategy> strategy_; 
    std::unique_ptr<PacketCapture>       capture_;
    PacketParser                         parser_;
    TrafficStatistics                    stats_;
    std::string                          reportPath_;
    std::unique_ptr<ReportGenerator>     reporter_;
};


TEST_F(ConsoleUITest, ConstructorNoThrow) {
    EXPECT_NO_THROW({
        ConsoleUI ui(*capture_, parser_, stats_, *reporter_);
    });
}


TEST_F(ConsoleUITest, RunExitsOnZero) {
    ConsoleUI ui(*capture_, parser_, stats_, *reporter_);

    StdinRedirect  in("0\n");
    StdoutCapture  out;

    EXPECT_NO_THROW(ui.run());
    EXPECT_NE(out.str().find("Exiting"), std::string::npos);
}


TEST_F(ConsoleUITest, ListInterfacesPrintsOutput) {
    ConsoleUI ui(*capture_, parser_, stats_, *reporter_);

    StdinRedirect  in("1\n0\n");
    StdoutCapture  out;
    ui.run();

    const std::string s = out.str();
    EXPECT_NE(s.find("INTERFACES"), std::string::npos);
}


TEST_F(ConsoleUITest, NonNumericInputHandled) {
    ConsoleUI ui(*capture_, parser_, stats_, *reporter_);

    StdinRedirect  in("abc\n0\n");
    StdoutCapture  out;

    EXPECT_NO_THROW(ui.run());
}


TEST_F(ConsoleUITest, ShowStatisticsWithNoData) {
    ConsoleUI ui(*capture_, parser_, stats_, *reporter_);

    StdinRedirect  in("5\n0\n");
    StdoutCapture  out;
    ui.run();

    const std::string s = out.str();
    EXPECT_NE(s.find("No data"), std::string::npos);
}


TEST_F(ConsoleUITest, ExportReportWithNoCaptureShowsWarning) {
    ConsoleUI ui(*capture_, parser_, stats_, *reporter_);

    StdinRedirect  in("6\n0\n");
    StdoutCapture  out;
    ui.run();

    const std::string s = out.str();

    EXPECT_NE(s.find("No packets"), std::string::npos);
}


TEST_F(ConsoleUITest, UnknownOptionShowsError) {
    ConsoleUI ui(*capture_, parser_, stats_, *reporter_);

    StdinRedirect  in("99\n0\n");
    StdoutCapture  out;
    ui.run();

    const std::string s = out.str();
    EXPECT_NE(s.find("Unknown"), std::string::npos);
}
