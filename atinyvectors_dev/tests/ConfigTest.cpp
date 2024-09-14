#include <gtest/gtest.h>
#include "Config.hpp"
#include <cstdlib>  // for setenv()

using namespace atinyvectors;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config& config = Config::getInstance();
        config.reset();

        // Ensure any previous environment variables are cleared
        unsetenv("ATV_HNSW_INDEX_CACHE_CAPACITY");
        unsetenv("ATV_DEFAULT_DB_NAME");
        unsetenv("ATV_LOG_FILE");
        unsetenv("ATV_LOG_LEVEL");
    }

    void TearDown() override {
        // Clean up environment variables after each test
        unsetenv("ATV_HNSW_INDEX_CACHE_CAPACITY");
        unsetenv("ATV_DEFAULT_DB_NAME");
        unsetenv("ATV_LOG_FILE");
        unsetenv("ATV_LOG_LEVEL");
    }
};

TEST_F(ConfigTest, TestDefaultValues) {
    Config& config = Config::getInstance();
    
    EXPECT_EQ(config.getHnswIndexCacheCapacity(), 100);
    EXPECT_EQ(config.getDefaultDbName(), ":memory:");
}

TEST_F(ConfigTest, TestEnvironmentVariableOverrides) {
    // Set environment variables
    setenv("ATV_HNSW_INDEX_CACHE_CAPACITY", "200", 1);  // Override cache capacity
    setenv("ATV_DEFAULT_DB_NAME", "custom_db.db", 1);   // Override default DB name

    // Get new instance of Config after setting env variables
    Config& config = Config::getInstance();

    // Check if values are overridden
    EXPECT_EQ(config.getHnswIndexCacheCapacity(), 200);
    EXPECT_EQ(config.getDefaultDbName(), "custom_db.db");
}

TEST_F(ConfigTest, TestInvalidEnvironmentVariables) {
    // Set invalid environment variables
    setenv("ATV_HNSW_INDEX_CACHE_CAPACITY", "invalid_value", 1);  // Invalid value for cache capacity

    // Get new instance of Config
    Config& config = Config::getInstance();

    // Ensure it falls back to the default value
    EXPECT_EQ(config.getHnswIndexCacheCapacity(), 100);  // Should use the default value
}
