#include <gtest/gtest.h>
#include "Config.hpp"
#include <cstdlib>

using namespace atinyvectors;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config::reset(); // Reset the config instance

        // Ensure any previous environment variables are cleared
        unsetenv("ATV_HNSW_INDEX_CACHE_CAPACITY");
        unsetenv("ATV_DB_NAME");
        unsetenv("ATV_LOG_FILE");
        unsetenv("ATV_LOG_LEVEL");
        unsetenv("ATV_DEFAULT_M");
        unsetenv("ATV_DEFAULT_EF_CONSTRUCTION");
        unsetenv("ATV_HNSW_MAX_DATASIZE");
    }

    void TearDown() override {
        // Clean up environment variables after each test
        unsetenv("ATV_HNSW_INDEX_CACHE_CAPACITY");
        unsetenv("ATV_DB_NAME");
        unsetenv("ATV_LOG_FILE");
        unsetenv("ATV_LOG_LEVEL");
        unsetenv("ATV_DEFAULT_M");
        unsetenv("ATV_DEFAULT_EF_CONSTRUCTION");
        unsetenv("ATV_HNSW_MAX_DATASIZE");
    }
};

TEST_F(ConfigTest, TestDefaultValues) {
    Config& config = Config::getInstance();
    
    EXPECT_EQ(config.getHnswIndexCacheCapacity(), 100);
    EXPECT_EQ(config.getDbName(), ":memory:");
    EXPECT_EQ(config.getM(), 16);
    EXPECT_EQ(config.getEfConstruction(), 100);
    EXPECT_EQ(config.getHnswMaxDataSize(), 1000000);
}

TEST_F(ConfigTest, TestEnvironmentVariableOverrides) {
    // Set environment variables
    setenv("ATV_HNSW_INDEX_CACHE_CAPACITY", "200", 1);  // Override cache capacity
    setenv("ATV_DB_NAME", "custom_db.db", 1);   // Override default DB name
    setenv("ATV_DEFAULT_M", "32", 1);  // Override M value
    setenv("ATV_DEFAULT_EF_CONSTRUCTION", "150", 1);  // Override EF_CONSTRUCTION value
    setenv("ATV_HNSW_MAX_DATASIZE", "2000000", 1);  // Override HNSW_MAX_DATASIZE value

    // Get new instance of Config after setting env variables
    Config& config = Config::getInstance();

    // Check if values are overridden
    EXPECT_EQ(config.getHnswIndexCacheCapacity(), 200);
    EXPECT_EQ(config.getDbName(), "custom_db.db");
    EXPECT_EQ(config.getM(), 32);
    EXPECT_EQ(config.getEfConstruction(), 150);
    EXPECT_EQ(config.getHnswMaxDataSize(), 2000000);
}

TEST_F(ConfigTest, TestInvalidEnvironmentVariables) {
    // Set invalid environment variables
    setenv("ATV_HNSW_INDEX_CACHE_CAPACITY", "invalid_value", 1);  // Invalid value for cache capacity
    setenv("ATV_DEFAULT_M", "invalid_value", 1);  // Invalid value for M
    setenv("ATV_DEFAULT_EF_CONSTRUCTION", "invalid_value", 1);  // Invalid value for EF_CONSTRUCTION
    setenv("ATV_HNSW_MAX_DATASIZE", "invalid_value", 1);  // Invalid value for HNSW_MAX_DATASIZE

    // Get new instance of Config
    Config& config = Config::getInstance();

    // Ensure it falls back to the default values
    EXPECT_EQ(config.getHnswIndexCacheCapacity(), 100);  // Should use the default value
    EXPECT_EQ(config.getM(), 16);  // Should use the default value
    EXPECT_EQ(config.getEfConstruction(), 100);  // Should use the default value
    EXPECT_EQ(config.getHnswMaxDataSize(), 1000000);  // Should use the default value
}
