// SearchTest.cpp
#include <gtest/gtest.h>
#include "service/VectorService.hpp"
#include "algo/FaissIndexLRUCache.hpp" 
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Snapshot.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "nlohmann/json.hpp"
#include <vector>

using namespace atinyvectors;
using namespace atinyvectors::service;
using namespace atinyvectors::algo;
using namespace nlohmann;

// Test Fixture class
class SearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        IdCache::getInstance().clean();

        // clean data
        DatabaseManager::getInstance().reset();
        FaissIndexLRUCache::getInstance().clean();
    }

    void TearDown() override {
    }
};

TEST_F(SearchTest, VectorSearchTest) {
    // Set up Space, Version, and VectorIndex for search
    Space defaultSpace(0, "VectorSearchTest", "Default Space Description", 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(defaultSpace);

    Version defaultVersion(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    // Use updated constructor to create VectorIndex
    HnswConfig hnswConfig(16, 200);  // Example HNSW config: M = 16, EfConstruct = 200
    QuantizationConfig quantizationConfig;  // Default empty quantization config

    VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", MetricType::L2, 4,
                             hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 0, 0, true);
    int indexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);

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

    VectorServiceManager manager;
    manager.upsert("VectorSearchTest", 0, vectorDataJson);

    // Prepare search query
    std::vector<float> queryVector = {0.25f, 0.45f, 0.75f, 0.85f};

    // Use the HnswIndexLRUCache to perform the search
    auto hnswIndexManager = FaissIndexLRUCache::getInstance().get(indexId);
    auto results = hnswIndexManager->search(queryVector, 2);

    // Check the search results
    ASSERT_EQ(results.size(), 2);  // Expect 2 results
}
