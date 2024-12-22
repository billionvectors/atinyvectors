#include <gtest/gtest.h>
#include "BM25.hpp"
#include "DatabaseManager.hpp"

using namespace atinyvectors;

class BM25Test : public ::testing::Test {
protected:
    void SetUp() override {
        DatabaseManager::getInstance().reset();
    }

    void TearDown() override {
    }
};

TEST_F(BM25Test, AddDocumentTest) {
    BM25Manager& bm25Manager = BM25Manager::getInstance();

    std::vector<std::string> tokens = {"token1", "token2", "token3"};
    bm25Manager.addDocument(1, "Document 1", tokens);

    // Check if the document is added
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT COUNT(*) FROM BM25");
    query.executeStep();

    int count = query.getColumn(0).getInt();
    ASSERT_EQ(count, 1);  // Verify one document was added
}

TEST_F(BM25Test, SearchWithVectorIdsTest) {
    BM25Manager& bm25Manager = BM25Manager::getInstance();

    // Add documents to the database
    bm25Manager.addDocument(1, "Document 1", {"token1", "token2", "token3"});
    bm25Manager.addDocument(2, "Document 2", {"token1", "token4", "token5"});

    // Perform a search with specific vector IDs
    std::vector<long> vectorIds = {1, 2};
    std::vector<std::string> queryTokens = {"token1", "token2"};

    auto results = bm25Manager.searchWithVectorIds(vectorIds, queryTokens);

    // Verify the search results
    ASSERT_EQ(results.size(), 2);  // Expect 2 results

    // Check the order of results (vectorId 1 should have a higher score)
    ASSERT_GT(results[0].second, results[1].second);
    ASSERT_EQ(results[0].first, 1);
    ASSERT_EQ(results[1].first, 2);
}

TEST_F(BM25Test, SearchEmptyVectorIdsTest) {
    BM25Manager& bm25Manager = BM25Manager::getInstance();

    // Add documents to the database
    bm25Manager.addDocument(1, "Document 1", {"token1", "token2", "token3"});
    bm25Manager.addDocument(2, "Document 2", {"token4", "token5", "token6"});

    // Perform a search with non-empty vector IDs
    std::vector<long> vectorIds = {1, 2};
    std::vector<std::string> queryTokens = {"token1"};

    auto results = bm25Manager.searchWithVectorIds(vectorIds, queryTokens);

    // Log results and verify
    for (const auto& [vectorId, score] : results) {
        std::cout << "Vector ID: " << vectorId << ", Score: " << score << std::endl;
    }

    // Verify results
    ASSERT_EQ(results.size(), 2);

    // Expected scores based on natural logarithm calculations
    double expectedScore1 = 0.693147;  // ln(2) for vectorId 1
    double expectedScore2 = 0.0;       // No match for token1 in document 2

    ASSERT_NEAR(results[0].second, expectedScore1, 0.01);
    ASSERT_NEAR(results[1].second, expectedScore2, 0.01);
}
