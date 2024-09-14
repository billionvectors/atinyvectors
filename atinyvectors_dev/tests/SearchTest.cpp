// SearchTest.cpp

#include <gtest/gtest.h>
#include "dto/VectorDTO.hpp"
#include "algo/HnswIndexManager.hpp"
#include "algo/HnswIndexLRUCache.hpp" 
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorIndexOptimizer.hpp"
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
class SearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        SpaceManager::getInstance();
        VectorIndexOptimizerManager::getInstance();
        VectorIndexManager::getInstance();
        VersionManager::getInstance();
        VectorManager::getInstance();
        DatabaseManager::getInstance();

        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndexOptimizer;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }

    void TearDown() override {
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndexOptimizer;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }
};

TEST_F(SearchTest, VectorSearchTest) {
    // Set up Space, Version, and VectorIndex for search
    Space defaultSpace(0, "default_space", "Default Space Description", 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(defaultSpace);

    Version defaultVersion(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", 0, 0, true);
    int indexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);

    // Add an entry to VectorIndexOptimizer for the search index
    SQLite::Statement optimizerQuery(DatabaseManager::getInstance().getDatabase(),
                                     "INSERT INTO VectorIndexOptimizer (vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson) "
                                     "VALUES (?, ?, ?, ?, ?)");
    optimizerQuery.bind(1, indexId);
    optimizerQuery.bind(2, static_cast<int>(MetricType::L2));  // Use L2 metric type
    optimizerQuery.bind(3, 128);  // Dimension size of 128
    optimizerQuery.bind(4, R"({"M": 16, "EfConstruct": 200})");  // HNSW config
    optimizerQuery.bind(5, "{}");  // Empty quantization config
    optimizerQuery.exec();

    // Insert vector data into the database
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

    VectorDTOManager manager;
    manager.upsert("default_space", 0, vectorDataJson);

    // Prepare search query
    std::vector<float> queryVector = {0.25f, 0.45f, 0.75f, 0.85f};

    // Use the HnswIndexLRUCache to perform the search
    auto hnswIndexManager = HnswIndexLRUCache::getInstance().get(indexId, "search_test_file3333", 128, 10000);
    auto results = hnswIndexManager->search(queryVector, 2);

    // Check the search results
    ASSERT_EQ(results.size(), 2);  // Expect 2 results

    // Check if the results are sorted in descending order of distance (closer first)
    EXPECT_LE(results[0].first, results[1].first);  // The first result should be closer or equal to the second result
}
