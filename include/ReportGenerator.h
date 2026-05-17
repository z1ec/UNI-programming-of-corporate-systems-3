#pragma once

#include "TrafficStatistics.h"
#include <string>

// ---------------------------------------------------------------------------
// ReportGenerator – produces a human-readable text summary of a capture
// session and writes it to a file.
//
// The output path defaults to "traffic_report.txt" in the working directory
// but can be overridden via the constructor.
// ---------------------------------------------------------------------------
class ReportGenerator {
public:
    explicit ReportGenerator(std::string outputPath = "traffic_report.txt");

    // Build the report text and write it to the output file.
    // Returns true on success, false if the file could not be opened.
    bool generateReport(const TrafficStatistics& stats) const;

    [[nodiscard]] const std::string& getOutputPath() const;

private:
    std::string outputPath_;

    // Build the full report string (separated for unit-testability).
    [[nodiscard]] std::string buildReportText(const TrafficStatistics& stats) const;
};
