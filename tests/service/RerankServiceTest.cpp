#include <gtest/gtest.h>
#include "service/RerankService.hpp"
#include "service/SpaceService.hpp"
#include "service/SearchService.hpp"
#include "service/BM25Service.hpp"
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
#include "RbacToken.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using namespace atinyvectors;
using namespace atinyvectors::service;
using namespace atinyvectors::algo;
using namespace nlohmann;

// Test Fixture class
class RerankServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        IdCache::getInstance().clean();

        // Clean data
        DatabaseManager::getInstance().reset();
        FaissIndexLRUCache::getInstance().clean();
    }

    void TearDown() override {
    }
};

TEST_F(RerankServiceTest, RerankWithBM25) {
    // Create a space
    std::string inputJson = R"({
        "name": "RerankWithBM25",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceServiceManager spaceServiceManager;
    spaceServiceManager.createSpace(inputJson);

    // Insert some vectors with documents for search and rerank
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"},
                "doc": "This is a test document about vectors.",
                "doc_tokens": ["test", "document", "vectors"]
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"},
                "doc": "Another document with different content.",
                "doc_tokens": ["another", "document", "different", "content"]
            }
        ]
    })";

    VectorServiceManager vectorServiceManager;
    vectorServiceManager.upsert("RerankWithBM25", 1, vectorDataJson);

    // Prepare search query JSON with tokens for BM25 rerank
    std::string queryJsonStr = R"({
        "vector": [0.25, 0.45, 0.75, 0.85],
        "tokens": ["test", "vectors"]
    })";

    // Create RerankServiceManager instance
    SearchServiceManager searchServiceManager;
    BM25ServiceManager bm25ServiceManager;
    RerankServiceManager rerankServiceManager(searchServiceManager, bm25ServiceManager);

    // Perform rerank
    auto rerankResults = rerankServiceManager.rerank("RerankWithBM25", 1, queryJsonStr, 10);

    // Log results for debugging
    spdlog::info("Rerank results size: {}", rerankResults.size());
    for (const auto& result : rerankResults) {
        spdlog::info("VectorUniqueId: {}, Distance: {}, BM25Score: {}", result["id"], result["distance"], result["bm25_score"]);
    }

    // Validate that two results are returned
    ASSERT_EQ(rerankResults.size(), 2);

    // Check that results are sorted by BM25 score descending, and distance ascending
    double bm25Score1 = rerankResults[0]["bm25_score"];
    double bm25Score2 = rerankResults[1]["bm25_score"];
    EXPECT_GE(bm25Score1, bm25Score2);

    // Validate vector unique IDs
    int vectorUniqueId1 = rerankResults[0]["id"];
    int vectorUniqueId2 = rerankResults[1]["id"];
    EXPECT_EQ(vectorUniqueId1, 1);
    EXPECT_EQ(vectorUniqueId2, 2);

    // Check distances
    double distance1 = rerankResults[0]["distance"];
    double distance2 = rerankResults[1]["distance"];
    EXPECT_LE(distance1, distance2);
}
