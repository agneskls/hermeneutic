#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/log/check.h"
#include "absl/log/log_sink.h"
#include "absl/log/log_sink_registry.h"
#include "absl/log/globals.h"
#include <fstream>

class file_log_sink : public absl::LogSink {
 public:
    explicit file_log_sink(const std::string& filename) {
        absl::SetStderrThreshold(absl::LogSeverity::kFatal); // to disable most console log
        log_file_.open(filename + ".log", std::ios::out | std::ios::app);
        absl::AddLogSink(this);
    }

    ~file_log_sink() override {
        if (log_file_.is_open()) log_file_.close();
        absl::RemoveLogSink(this);
    }

    void Send(const absl::LogEntry& entry) override {
        if (!log_file_.is_open()) return;

        std::ostringstream oss;
        oss << absl::FormatTime(absl::RFC3339_full, entry.timestamp(), absl::LocalTimeZone())
            << " [" << absl::LogSeverityName(entry.log_severity()) << "] "
            << entry.source_filename() << ":" << entry.source_line() << " "
            << entry.text_message();

        log_file_ << oss.str() << std::endl;
    }

    void set_log_level(absl::LogSeverity level) {
        absl::SetStderrThreshold(level);
    }

 private:
    std::ofstream log_file_;
};


#endif