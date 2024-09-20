#include <gtest/gtest.h>
#include "dto/SpaceDTO.hpp"
#include "dto/SearchDTO.hpp"
#include "dto/VectorDTO.hpp"
#include "algo/HnswIndexLRUCache.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Snapshot.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "RbacToken.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include <vector>
#include <cmath>

using namespace atinyvectors;
using namespace atinyvectors::dto;
using namespace atinyvectors::algo;
using namespace nlohmann;

// Test Fixture class
class SearchDTOTest : public ::testing::Test {
protected:
    void SetUp() override {
        IdCache::getInstance().clean();

        SnapshotManager& snapshotManager = SnapshotManager::getInstance();
        snapshotManager.createTable();

        SpaceManager& spaceManager = SpaceManager::getInstance();
        spaceManager.createTable();
        
        VectorIndexManager& vectorIndexManager = VectorIndexManager::getInstance();
        vectorIndexManager.createTable();

        VersionManager& versionManager = VersionManager::getInstance();
        versionManager.createTable();

        VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();
        metadataManager.createTable();

        VectorManager& vectorManager = VectorManager::getInstance();
        vectorManager.createTable();

        RbacTokenManager& rbacTokenManager = RbacTokenManager::getInstance();
        rbacTokenManager.createTable();

        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM RbacToken;");

        // clean data
        HnswIndexLRUCache::getInstance().clean();
    }

    void TearDown() override {
        // Clear data after each test
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM RbacToken;");
    }
};

TEST_F(SearchDTOTest, VectorSearchWithJSONQuery) {
    // Set up Space, Version, and VectorIndex for search
    Space defaultSpace(0, "VectorSearchWithJSONQuery", "Default Space Description", 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(defaultSpace);

    // Use versionUniqueId as 1 for the test case
    Version defaultVersion(0, spaceId, 1, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    // Create default HNSW and Quantization configurations
    HnswConfig hnswConfig(16, 200);  // Example HNSW config: M = 16, EfConstruct = 200
    QuantizationConfig quantizationConfig;  // Default empty quantization config

    // Create and add a default VectorIndex
    VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", MetricType::L2, 4,
                             hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 0, 0, true);
    int indexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);

    // Insert some vectors for search
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"}
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"}
            }
        ]
    })";

    VectorDTOManager vectorDTOManager;
    vectorDTOManager.upsert("VectorSearchWithJSONQuery", 1, vectorDataJson);

    // Prepare search query JSON
    std::string queryJsonStr = R"({
        "vector": [0.25, 0.45, 0.75, 0.85]
    })";

    // Perform the search using SearchDTOManager
    SearchDTOManager searchManager;
    auto searchResults = searchManager.search("VectorSearchWithJSONQuery", 1, queryJsonStr, 2);

    // Log distances and result metadata for debugging
    spdlog::info("Search results size: {}", searchResults.size());
    for (const auto& result : searchResults) {
        spdlog::info("Distance: {}, ID: {}", result.first, result.second);
    }

    // Validate that two results are returned
    ASSERT_EQ(searchResults.size(), 2);  

    // Validate distances
    double distance1 = searchResults[0].first;
    double distance2 = searchResults[1].first;

    // Log distances for debugging
    spdlog::info("Distance1: {}", distance1);
    spdlog::info("Distance2: {}", distance2);

    // Expected distances
    double expectedDistance1 = 0.0; // Distance is 0 for the same vector
    double expectedDistance2 = (0.05 * 0.05 + 0.17 * 0.17 + 0.02 * 0.02 + 0.10 * 0.10); // L2 distance to the second vector

    EXPECT_NEAR(distance1, expectedDistance1, 1e-6); // Compare within allowable error range
    EXPECT_NEAR(distance2, expectedDistance2, 1e-6);

    // Check if metadata is intact for the found vectors
    auto vector1 = VectorManager::getInstance().getVectorByUniqueId(versionId, 1);
    auto metadataList1 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector1.id);
    spdlog::info("Vector1 metadata: key = {}, value = {}", metadataList1[0].key, metadataList1[0].value);
    EXPECT_EQ(metadataList1[0].key, "category");
    EXPECT_EQ(metadataList1[0].value, "A");

    auto vector2 = VectorManager::getInstance().getVectorByUniqueId(versionId, 2);
    auto metadataList2 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector2.id);
    spdlog::info("Vector2 metadata: key = {}, value = {}", metadataList2[0].key, metadataList2[0].value);
    EXPECT_EQ(metadataList2[0].key, "category");
    EXPECT_EQ(metadataList2[0].value, "B");
}

TEST_F(SearchDTOTest, VectorSearchWithJSONQueryUsingDefaultVersion) {
    // Create a space
    std::string inputJson = R"({
        "name": "VectorSearchWithDefaultVersion",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceDTOManager manager;
    manager.createSpace(inputJson);

    // Insert some vectors for search
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"}
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"}
            }
        ]
    })";

    VectorDTOManager vectorDTOManager;
    vectorDTOManager.upsert("VectorSearchWithDefaultVersion", 1, vectorDataJson); // versionUniqueId is 1

    // Prepare search query JSON
    std::string queryJsonStr = R"({
        "vector": [0.25, 0.45, 0.75, 0.85]
    })";

    SearchDTOManager searchManager;
    auto searchResults = searchManager.search("VectorSearchWithDefaultVersion", 0, queryJsonStr, 2); // default version

    // Log distances and result metadata for debugging
    spdlog::info("Search results size: {}", searchResults.size());
    for (const auto& result : searchResults) {
        spdlog::info("Distance: {}, ID: {}", result.first, result.second);
    }

    // Validate that two results are returned
    ASSERT_EQ(searchResults.size(), 2);

    // Validate distances
    double distance1 = searchResults[0].first;
    double distance2 = searchResults[1].first;

    // Log distances for debugging
    spdlog::info("Distance1: {}", distance1);
    spdlog::info("Distance2: {}", distance2);

    // Expected distances
    double expectedDistance1 = 0.0; // Distance is 0 for the same vector
    double expectedDistance2 = (0.05 * 0.05 + 0.17 * 0.17 + 0.02 * 0.02 + 0.10 * 0.10); // L2 distance to the second vector

    EXPECT_NEAR(distance1, expectedDistance1, 1e-6); // Compare within allowable error range
    EXPECT_NEAR(distance2, expectedDistance2, 1e-6);
}
