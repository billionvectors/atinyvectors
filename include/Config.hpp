#ifndef __ATINYVECTORS_CONFIG_HPP__
#define __ATINYVECTORS_CONFIG_HPP__

#include <iostream>
#include <string>
#include <cstdlib>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace atinyvectors {

class Config {
public:
    static Config& getInstance() {
        if (!instance_) {
            instance_.reset(new Config());
        }
        return *instance_;
    }

    static void reset() {
        instance_.reset(nullptr);  // Reset the instance, forcing reinitialization on next getInstance() call
    }

    int getHnswIndexCacheCapacity() const {
        return hnswIndexCacheCapacity_;
    }

    std::string getDefaultDbName() const {
        return defaultDbName_;
    }

    void initializeLogger() {
        spdlog::level::level_enum logLevel = spdlog::level::info;
        if (logLevel_ == "debug") {
            logLevel = spdlog::level::debug;
        } else if (logLevel_ == "warn") {
            logLevel = spdlog::level::warn;
        } else if (logLevel_ == "error") {
            logLevel = spdlog::level::err;
        }

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile_, true);
        std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};

        auto logger = std::make_shared<spdlog::logger>("atinyvectors_logger", begin(sinks), end(sinks));
        spdlog::set_default_logger(logger);
        spdlog::set_level(logLevel);
        spdlog::flush_on(logLevel);

        spdlog::info("Logger initialized. Log level: {}, Log file: {}", logLevel_, logFile_);
    }

private:
    static std::unique_ptr<Config> instance_;  // Singleton instance

    const int defaultHnswIndexCacheCapacity = 100;
    const std::string defaultDbName = ":memory:";
    const std::string defaultLogFile = "logs/atinyvectors.log";
    const std::string defaultLogLevel = "info";

    int hnswIndexCacheCapacity_;  // Cache capacity for HNSW index
    std::string defaultDbName_;   // Default DB name
    std::string logFile_;         // Log file path
    std::string logLevel_;        // Log level

    Config() {
        const char* envCacheCapacity = std::getenv("ATV_HNSW_INDEX_CACHE_CAPACITY");
        const char* envDbName = std::getenv("ATV_DEFAULT_DB_NAME");
        const char* envLogFile = std::getenv("ATV_LOG_FILE");
        const char* envLogLevel = std::getenv("ATV_LOG_LEVEL");

        // Use default if environment variable is invalid
        try {
            hnswIndexCacheCapacity_ = (envCacheCapacity) ? std::stoi(envCacheCapacity) : defaultHnswIndexCacheCapacity;
        } catch (...) {
            spdlog::warn("Invalid value for ATV_HNSW_INDEX_CACHE_CAPACITY. Using default value: {}", defaultHnswIndexCacheCapacity);
            hnswIndexCacheCapacity_ = defaultHnswIndexCacheCapacity;
        }

        defaultDbName_ = (envDbName) ? envDbName : defaultDbName;
        logFile_ = (envLogFile) ? envLogFile : defaultLogFile;
        logLevel_ = (envLogLevel) ? envLogLevel : defaultLogLevel;

        spdlog::info("Config initialized - HNSW Cache Capacity: {}, Default DB Name: {}, Log File: {}, Log Level: {}",
                     hnswIndexCacheCapacity_, defaultDbName_, logFile_, logLevel_);
    }

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

}  // namespace atinyvectors

#endif
