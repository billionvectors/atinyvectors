#include "algo/HnswIndexLRUCache.hpp"
#include "gtest/gtest.h"
#include "DatabaseManager.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorIndexOptimizer.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "spdlog/spdlog.h"

using namespace atinyvectors;
using namespace atinyvectors::algo;

// Test Fixture for HnswIndexLRUCache
class HnswIndexLRUCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        SpaceManager::getInstance();
        VectorIndexOptimizerManager::getInstance();
        VectorIndexManager::getInstance();
        VersionManager::getInstance();
        VectorMetadataManager::getInstance();
        VectorManager::getInstance();

        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndexOptimizer;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");

        indexFileName = "test_hnsw_index_cache.bin";
        dim = 16;
        maxElements = 1000;

        createMockDatas(db); // Create mock data for testing
    }

    void TearDown() override {
        std::remove(indexFileName.c_str());
        
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndexOptimizer;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }

    void createMockDatas(SQLite::Database& db) {
        // Insert mock data into VectorIndexOptimizer
        db.exec("INSERT INTO VectorIndexOptimizer (vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson) "
                "VALUES (1, 0, 16, '{\"M\": 16, \"EfConstruct\": 200}', '{}')");  // metricType: 0 (L2 distance)
        db.exec("INSERT INTO VectorIndexOptimizer (vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson) "
                "VALUES (2, 2, 16, '{\"M\": 16, \"EfConstruct\": 200}', '{}')");  // metricType: 2 (InnerProduct distance)
        db.exec("INSERT INTO VectorIndexOptimizer (vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson) "
                "VALUES (3, 1, 16, '{\"M\": 16, \"EfConstruct\": 200}', '{}')");  // metricType: 1 (Cosine distance)
    }

    std::string indexFileName;
    int dim;
    int maxElements;
};

TEST_F(HnswIndexLRUCacheTest, TestGetAndPut) {
    auto& cache = HnswIndexLRUCache::getInstance();
    auto manager = cache.get(1, indexFileName, dim, maxElements);

    // Basic check that the manager is fetched without error
    ASSERT_NE(manager, nullptr);
}

TEST_F(HnswIndexLRUCacheTest, TestReAccessEvictedInstance) {
    auto& cache = HnswIndexLRUCache::getInstance();

    auto manager1 = cache.get(1, indexFileName, dim, maxElements);
    cache.get(2, indexFileName, dim, maxElements);
    cache.get(3, indexFileName, dim, maxElements);

    auto manager2 = cache.get(1, indexFileName, dim, maxElements);
    ASSERT_EQ(manager1, manager2);
}
