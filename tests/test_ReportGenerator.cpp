#include <gtest/gtest.h>
#include "ReportGenerator.h"
#include "TrafficStatistics.h"
#include "PacketInfo.h"

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

const std::string kTmpDir = (fs::temp_directory_path() / "sniffer_tests").string();

std::string tmpPath(const std::string& name) {
    return kTmpDir + "/" + name;
}

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

TrafficStatistics makeStats() {
    TrafficStatistics s;
    PacketInfo p;
    p.sourceIp      = "192.168.1.1";
    p.destinationIp = "8.8.8.8";
    p.protocol      = Protocol::TCP;
    p.sizeBytes     = 1500;
    s.addPacket(p);

    PacketInfo p2;
    p2.sourceIp      = "192.168.1.2";
    p2.destinationIp = "8.8.4.4";
    p2.protocol      = Protocol::UDP;
    p2.sizeBytes     = 128;
    s.addPacket(p2);

    return s;
}

} 

class ReportGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs::create_directories(kTmpDir);
    }
};


TEST_F(ReportGeneratorTest, GetOutputPathMatchesConstructor) {
    ReportGenerator rg("/tmp/test_out.txt");
    EXPECT_EQ(rg.getOutputPath(), "/tmp/test_out.txt");
}


TEST_F(ReportGeneratorTest, DefaultOutputPath) {
    ReportGenerator rg;
    EXPECT_EQ(rg.getOutputPath(), "traffic_report.txt");
}


TEST_F(ReportGeneratorTest, GenerateReportCreatesFile) {
    const std::string path = tmpPath("report_create.txt");
    ReportGenerator rg(path);
    TrafficStatistics stats = makeStats();

    EXPECT_TRUE(rg.generateReport(stats));
    EXPECT_TRUE(fs::exists(path));
}


TEST_F(ReportGeneratorTest, ReportContainsHeader) {
    const std::string path = tmpPath("report_header.txt");
    ReportGenerator rg(path);
    ASSERT_TRUE(rg.generateReport(makeStats()));

    const std::string content = readFile(path);
    EXPECT_NE(content.find("TRAFFIC REPORT"), std::string::npos);
    EXPECT_NE(content.find("GLOBAL TOTALS"),  std::string::npos);
}


TEST_F(ReportGeneratorTest, ReportContainsProtocols) {
    const std::string path = tmpPath("report_proto.txt");
    ReportGenerator rg(path);
    ASSERT_TRUE(rg.generateReport(makeStats()));

    const std::string content = readFile(path);
    EXPECT_NE(content.find("TCP"), std::string::npos);
    EXPECT_NE(content.find("UDP"), std::string::npos);
}


TEST_F(ReportGeneratorTest, EmptyStatsReport) {
    const std::string path = tmpPath("report_empty.txt");
    ReportGenerator rg(path);
    TrafficStatistics empty;

    EXPECT_TRUE(rg.generateReport(empty));
    const std::string content = readFile(path);
    EXPECT_NE(content.find("0"), std::string::npos);
}


TEST_F(ReportGeneratorTest, InvalidPathReturnsFalse) {
    ReportGenerator rg("/nonexistent_dir/x/y/z/report.txt");
    TrafficStatistics stats;
    EXPECT_FALSE(rg.generateReport(stats));
}
