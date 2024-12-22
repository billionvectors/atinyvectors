#ifndef __ATINYVECTORS_CONFIG_HPP__
#define __ATINYVECTORS_CONFIG_HPP__

#include <iostream>
#include <string>
#include <cstdlib>
#include <memory>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#ifndef ATINYVECTORS_PROJECT_VERSION
#define ATINYVECTORS_PROJECT_VERSION "0.2.0"
#endif

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
    
    std::string getProjectVersion() const {
        return ATINYVECTORS_PROJECT_VERSION;
    }

    int getHnswIndexCacheCapacity() const {
        return hnswIndexCacheCapacity_;
    }

    std::string getDbName() const {
        return dbName_;
    }

    int getM() const {
        return m_;
    }

    int getEfConstruction() const {
        return efConstruction_;
    }

    int getHnswMaxDataSize() const {
        return hnswMaxDataSize_;
    }

    std::string getDataPath() const {
        return dataPath_;
    }

    int getDefaultTokenExpireDays() const {
        return defaultTokenExpireDays_;
    }

    std::string getJwtTokenKey() const {
        return jwtTokenKey_;
    }

    std::string getDefaultDenseIndexName() const {
        return DEFAULT_DENSE_INDEX_NAME;
    }

    std::string getDefaultSparseIndexName() const {
        return DEFAULT_SPARSE_INDEX_NAME;
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
        spdlog::enable_backtrace(32);

        spdlog::info("Logger initialized. Log level: {}, Log file: {}", logLevel_, logFile_);
    }

private:
    static std::unique_ptr<Config> instance_;  // Singleton instance

    const int DEFAULT_HNSW_INDEX_CACHE_CAPACITY = 100;
    const int DEFAULT_M = 16;
    const int DEFAULT_EF_CONSTRUCTION = 100;
    const int DEFAULT_HNSW_MAX_DATASIZE = 1000000;
    const std::string DEFAULT_DB_NAME = ":memory:";
    const std::string DEFAULT_LOG_FILE = "logs/atinyvectors.log";
    const std::string DEFAULT_LOG_LEVEL = "info";
    const std::string DEFAULT_DATA_PATH = "data/";
    
    const int DEFAULT_TOKEN_EXPIRE_DAYS = 30;
    const std::string DEFAULT_JWT_TOKEN_KEY = "atinyvectors_jwt_token_key_is_really_good_and_i_hope_so_much_whatever_you_want";

    const std::string DEFAULT_DENSE_INDEX_NAME = "dense";
    const std::string DEFAULT_SPARSE_INDEX_NAME = "sparse";

    int hnswIndexCacheCapacity_;  // Cache capacity for HNSW index
    int m_;                       // M value for HNSW
    int efConstruction_;          // EF_CONSTRUCTION value for HNSW
    int hnswMaxDataSize_;         // Maximum data size for HNSW
    std::string dbName_;          // DB name
    std::string logFile_;         // Log file path
    std::string logLevel_;        // Log level
    std::string dataPath_;        // Data path
    int defaultTokenExpireDays_;  // Default token expire days
    std::string jwtTokenKey_;     // JWT token key

    Config() {
        const char* envCacheCapacity = std::getenv("ATV_HNSW_INDEX_CACHE_CAPACITY");
        const char* envDbName = std::getenv("ATV_DB_NAME");
        const char* envLogFile = std::getenv("ATV_LOG_FILE");
        const char* envLogLevel = std::getenv("ATV_LOG_LEVEL");
        const char* envM = std::getenv("ATV_DEFAULT_M");
        const char* envEfConstruction = std::getenv("ATV_DEFAULT_EF_CONSTRUCTION");
        const char* envMaxDataSize = std::getenv("ATV_HNSW_MAX_DATASIZE");
        const char* envDataPath = std::getenv("ATV_DATA_PATH");
        const char* envTokenExpireDays = std::getenv("ATV_DEFAULT_TOKEN_EXPIRE_DAYS");
        const char* envJwtTokenKey = std::getenv("ATV_JWT_TOKEN_KEY");

        // Use default if environment variable is invalid
        try {
            hnswIndexCacheCapacity_ = (envCacheCapacity) ? std::stoi(envCacheCapacity) : DEFAULT_HNSW_INDEX_CACHE_CAPACITY;
        } catch (...) {
            spdlog::warn("Invalid value for ATV_HNSW_INDEX_CACHE_CAPACITY. Using default value: {}", DEFAULT_HNSW_INDEX_CACHE_CAPACITY);
            hnswIndexCacheCapacity_ = DEFAULT_HNSW_INDEX_CACHE_CAPACITY;
        }

        try {
            m_ = (envM) ? std::stoi(envM) : DEFAULT_M;
        } catch (...) {
            spdlog::warn("Invalid value for ATV_M. Using default value: {}", DEFAULT_M);
            m_ = DEFAULT_M;
        }

        try {
            efConstruction_ = (envEfConstruction) ? std::stoi(envEfConstruction) : DEFAULT_EF_CONSTRUCTION;
        } catch (...) {
            spdlog::warn("Invalid value for ATV_EF_CONSTRUCTION. Using default value: {}", DEFAULT_EF_CONSTRUCTION);
            efConstruction_ = DEFAULT_EF_CONSTRUCTION;
        }

        try {
            hnswMaxDataSize_ = (envMaxDataSize) ? std::stoi(envMaxDataSize) : DEFAULT_HNSW_MAX_DATASIZE;
        } catch (...) {
            spdlog::warn("Invalid value for ATV_HNSW_MAX_DATASIZE. Using default value: {}", DEFAULT_HNSW_MAX_DATASIZE);
            hnswMaxDataSize_ = DEFAULT_HNSW_MAX_DATASIZE;
        }

        try {
            defaultTokenExpireDays_ = (envTokenExpireDays) ? std::stoi(envTokenExpireDays) : DEFAULT_TOKEN_EXPIRE_DAYS;
        } catch (...) {
            spdlog::warn("Invalid value for ATV_DEFAULT_TOKEN_EXPIRE_DAYS. Using default value: {}", DEFAULT_TOKEN_EXPIRE_DAYS);
            defaultTokenExpireDays_ = DEFAULT_TOKEN_EXPIRE_DAYS;
        }

        jwtTokenKey_ = (envJwtTokenKey) ? envJwtTokenKey : DEFAULT_JWT_TOKEN_KEY;

        dbName_ = (envDbName) ? envDbName : DEFAULT_DB_NAME;
        logFile_ = (envLogFile) ? envLogFile : DEFAULT_LOG_FILE;
        logLevel_ = (envLogLevel) ? envLogLevel : DEFAULT_LOG_LEVEL;
        dataPath_ = (envDataPath) ? envDataPath : DEFAULT_DATA_PATH;
        if (!dataPath_.empty() && dataPath_.back() != '/') {
            dataPath_ += '/';
        }

        // Ensure the data path directory exists
        try {
            if (!std::filesystem::exists(dataPath_)) {
                std::filesystem::create_directories(dataPath_);
                spdlog::debug("Created data directory: {}", dataPath_);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            spdlog::error("Error creating data directory {}: {}", dataPath_, e.what());
        }

        spdlog::debug("Config initialized - HNSW Cache Capacity: {}, M: {}, EF_CONSTRUCTION: {}, HNSW Max Data Size: {}, Default DB Name: {}, Log File: {}, Log Level: {}, Data Path: {}, Default Token Expire Days: {}, JWT Token Key: {}",
                     hnswIndexCacheCapacity_, m_, efConstruction_, hnswMaxDataSize_, dbName_, logFile_, logLevel_, dataPath_, defaultTokenExpireDays_, jwtTokenKey_);

        initializeLogger();
    }

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

}  // namespace atinyvectors

#endif
