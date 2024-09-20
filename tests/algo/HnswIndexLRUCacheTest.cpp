#include "algo/HnswIndexLRUCache.hpp"
#include "gtest/gtest.h"
#include "DatabaseManager.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
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
        VectorIndexManager::getInstance();
        VersionManager::getInstance();
        VectorMetadataManager::getInstance();
        VectorManager::getInstance();

        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");

        dim = 16;

        createDummyData();
    }

    void TearDown() override {
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }

    void createDummyData() {
        auto& db = DatabaseManager::getInstance().getDatabase();

        // Create a default space
        Space space(0, "HnswIndexLRUCacheTest", "Default Space Description", 0, 0);
        int spaceId = SpaceManager::getInstance().addSpace(space);

        // Create a default version
        Version version(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
        int versionId = VersionManager::getInstance().addVersion(version);

        // Define HNSW and quantization configurations
        HnswConfig hnswConfig(16, 200);
        QuantizationConfig quantizationConfig;

        // Insert mock data into VectorIndex
        VectorIndex vectorIndex1(0, versionId, VectorValueType::Dense, "Vector Index 1", MetricType::L2, dim,
                                 hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
        vectorIndexIds[0] = VectorIndexManager::getInstance().addVectorIndex(vectorIndex1);

        VectorIndex vectorIndex2(0, versionId, VectorValueType::Sparse, "Vector Index 2", MetricType::InnerProduct, dim,
                                 hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
        vectorIndexIds[1] = VectorIndexManager::getInstance().addVectorIndex(vectorIndex2);

        VectorIndex vectorIndex3(0, versionId, VectorValueType::Combined, "Vector Index 3", MetricType::Cosine, dim,
                                 hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
        vectorIndexIds[2] = VectorIndexManager::getInstance().addVectorIndex(vectorIndex3);

        VectorIndex vectorIndex4(0, versionId, VectorValueType::Combined, "Vector Index 4", MetricType::Cosine, dim,
                                 hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
        vectorIndexIds[3] = VectorIndexManager::getInstance().addVectorIndex(vectorIndex4);
    }

    int dim;
    int vectorIndexIds[4];
};

TEST_F(HnswIndexLRUCacheTest, TestGetAndPut) {
    auto& cache = HnswIndexLRUCache::getInstance();
    auto manager = cache.get(vectorIndexIds[0]);

    // Basic check that the manager is fetched without error
    ASSERT_NE(manager, nullptr);
}

TEST_F(HnswIndexLRUCacheTest, TestReAccessEvictedInstance) {
    auto& cache = HnswIndexLRUCache::getInstance();

    auto manager1 = cache.get(vectorIndexIds[0]);
    cache.get(vectorIndexIds[1]);
    cache.get(vectorIndexIds[2]);

    auto manager2 = cache.get(vectorIndexIds[0]);
    ASSERT_EQ(manager1, manager2);
}
