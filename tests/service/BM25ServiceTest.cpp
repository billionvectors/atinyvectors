#include <gtest/gtest.h>
#include "service/BM25Service.hpp"
#include "service/SnapshotService.hpp"
#include "service/SearchService.hpp"
#include "service/SpaceService.hpp"
#include "service/VectorService.hpp"
#include "algo/FaissIndexLRUCache.hpp"
#include "BM25.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Snapshot.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "spdlog/spdlog.h"

using namespace atinyvectors;
using namespace atinyvectors::service;

class BM25ServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        IdCache::getInstance().clean();
        DatabaseManager::getInstance().reset();
    }

    void TearDown() override {
    }
};

TEST_F(BM25ServiceTest, AddDocumentTest) {
    BM25ServiceManager service;

    // Create a space
    std::string inputJson = R"({
        "name": "TestSpace",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceServiceManager spaceServiceManager;
    spaceServiceManager.createSpace(inputJson);

    // Insert vectors for search
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

    VectorServiceManager vectorServiceManager;
    vectorServiceManager.upsert("TestSpace", 1, vectorDataJson);

    // Add a document
    service.addDocument("TestSpace", 1, 1, "Document 1", {"token1", "token2", "token3"});

    // Check if the document is added to the database
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT COUNT(*) FROM BM25");
    query.executeStep();

    int count = query.getColumn(0).getInt();
    ASSERT_EQ(count, 1); // Verify one document was added
}

TEST_F(BM25ServiceTest, SearchWithVectorUniqueIdsTest) {
    BM25ServiceManager service;

    // Create a space
    std::string inputJson = R"({
        "name": "TestSpace",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceServiceManager spaceServiceManager;
    spaceServiceManager.createSpace(inputJson);

    // Insert vectors for search
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

    VectorServiceManager vectorServiceManager;
    vectorServiceManager.upsert("TestSpace", 1, vectorDataJson);

    // Add documents
    service.addDocument("TestSpace", 1, 1, "Document 1", {"token1", "token2", "token3"});
    service.addDocument("TestSpace", 1, 2, "Document 2", {"token1", "token4", "token5"});

    // Perform a search with vectorUniqueIds
    std::vector<long> vectorUniqueIds = {1, 2};
    std::vector<std::string> queryTokens = {"token1", "token2"};

    auto resultsJson = service.searchWithVectorUniqueIdsToJson("TestSpace", 1, vectorUniqueIds, queryTokens);

    // Log the results
    spdlog::info("Search results: {}", resultsJson.dump(4));

    // Parse JSON and validate results
    ASSERT_EQ(resultsJson.size(), 2);

    // Verify the first result (highest score)
    EXPECT_EQ(resultsJson[0]["vectorUniqueId"], 1);
    EXPECT_GT(resultsJson[0]["score"], resultsJson[1]["score"]);

    // Verify the second result
    EXPECT_EQ(resultsJson[1]["vectorUniqueId"], 2);
}
