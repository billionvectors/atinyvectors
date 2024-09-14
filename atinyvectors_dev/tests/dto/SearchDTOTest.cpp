#include <gtest/gtest.h>
#include "dto/SearchDTO.hpp"
#include "dto/VectorDTO.hpp"
#include "algo/HnswIndexLRUCache.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorIndexOptimizer.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "DatabaseManager.hpp"
#include "nlohmann/json.hpp"
#include <vector>

using namespace atinyvectors;
using namespace atinyvectors::dto;
using namespace atinyvectors::algo;
using namespace nlohmann;

// Test Fixture class
class SearchDTOTest : public ::testing::Test {
protected:
    void SetUp() override {
        SpaceManager::getInstance();
        VectorIndexOptimizerManager::getInstance();
        VectorIndexManager::getInstance();
        VersionManager::getInstance();
        VectorManager::getInstance();
        VectorMetadataManager::getInstance();
        DatabaseManager::getInstance();

        // Clear previous data
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndexOptimizer;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM VectorMetadata;");
    }

    void TearDown() override {
        // Clear data after each test
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndexOptimizer;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM VectorMetadata;");
    }
};

TEST_F(SearchDTOTest, VectorSearchWithJSONQuery) {
    // Set up Space, Version, and VectorIndex for search
    Space defaultSpace(0, "default_space", "Default Space Description", 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(defaultSpace);

    Version defaultVersion(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", 0, 0, true);
    int indexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);

    // Add VectorIndexOptimizer for the vector index
    SQLite::Statement optimizerQuery(DatabaseManager::getInstance().getDatabase(),
                                     "INSERT INTO VectorIndexOptimizer (vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson) "
                                     "VALUES (?, ?, ?, ?, ?)");
    optimizerQuery.bind(1, indexId);
    optimizerQuery.bind(2, static_cast<int>(MetricType::L2));  // Use L2 metric type
    optimizerQuery.bind(3, 128);  // Dimension size of 128
    optimizerQuery.bind(4, R"({"M": 16, "EfConstruct": 200})");  // Example HNSW config
    optimizerQuery.bind(5, "{}");  // Empty quantization config
    optimizerQuery.exec();

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
    vectorDTOManager.upsert("default_space", 0, vectorDataJson);

    // Prepare search query JSON
    std::string queryJsonStr = R"({
        "vector": [0.25, 0.45, 0.75, 0.85]
    })";

    // Perform the search using SearchDTOManager
    SearchDTOManager searchManager;
    auto searchResults = searchManager.search("default_space", queryJsonStr, 2);

    // Validate that two results are returned
    ASSERT_EQ(searchResults.size(), 2);  

    // Since the order may differ, compare based on distances and ensure IDs are as expected
    std::vector<int> returnedIds = {static_cast<int>(searchResults[0].second), static_cast<int>(searchResults[1].second)};
    std::sort(returnedIds.begin(), returnedIds.end());  // Sort the IDs to handle any order
    std::vector<int> expectedIds = {1, 2};
    EXPECT_EQ(returnedIds, expectedIds);

    // Check if metadata is intact for the found vectors
    auto vector1 = VectorManager::getInstance().getVectorByUniqueId(versionId, 1);
    auto metadataList1 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector1.id);
    EXPECT_EQ(metadataList1[0].key, "category");
    EXPECT_EQ(metadataList1[0].value, "A");

    auto vector2 = VectorManager::getInstance().getVectorByUniqueId(versionId, 2);
    auto metadataList2 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector2.id);
    EXPECT_EQ(metadataList2[0].key, "category");
    EXPECT_EQ(metadataList2[0].value, "B");
}
