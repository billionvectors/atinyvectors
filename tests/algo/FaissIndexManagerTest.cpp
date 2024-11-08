// FaissIndexManagerTest.cpp

#include "algo/FaissIndexManager.hpp" // Update the include path as necessary
#include "DatabaseManager.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Config.hpp"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include <cmath>

using namespace atinyvectors;
using namespace atinyvectors::algo;
using namespace nlohmann;

// Test Fixture for FaissIndexManager
class FaissIndexManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize singleton managers
        SpaceManager::getInstance();
        VectorIndexManager::getInstance();
        VersionManager::getInstance();
        VectorMetadataManager::getInstance();
        VectorManager::getInstance();

        // Clear relevant tables to ensure a clean state
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");

        // Setup test parameters
        indexFileName = "test_faiss_index.bin";
        dim = 16;
        maxElements = 1000;

        createMockDatas();

        // Create the index manager with a specific MetricType (e.g., L2)
        HnswConfig config = HnswConfig(16, 200); // M=16, EfConstruct=200, EfSearch=200

        indexManager = std::make_unique<FaissIndexManager>(
            indexFileName,
            vectorIndexId,
            dim,
            maxElements,
            MetricType::L2,
            VectorValueType::Dense,
            config,
            QuantizationConfig()
        );
    }

    void TearDown() override {
        // Remove the index file after tests
        std::remove(indexFileName.c_str());

        // Clear relevant tables
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }

    void createMockDatas() {
        auto& db = DatabaseManager::getInstance().getDatabase();

        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM VectorIndex;");

        // Insert mock data into VectorIndex table with FAISS HNSW configuration
        db.exec("INSERT INTO VectorIndex (id, versionId, vectorValueType, name, metricType, dimension, hnswConfigJson, quantizationConfigJson, create_date_utc, updated_time_utc, is_default) "
                "VALUES (1, 1, 0, 'Test Index', 0, 16, '{\"M\": 16, \"EfConstruct\": 200, \"EfSearch\": 50}', '{}', 1627906032, 1627906032, 1)");
        vectorIndexId = static_cast<int>(db.getLastInsertRowid());

        SQLite::Statement insertVector(db, "INSERT INTO Vector (versionId, unique_id, type, deleted) VALUES (?, ?, ?, ?)");
        SQLite::Statement insertVectorValue(db, "INSERT INTO VectorValue (vectorId, vectorIndexId, type, data) VALUES (?, ?, ?, ?)");

        for (int i = 0; i < 10; ++i) {
            std::vector<float> data(dim, static_cast<float>(i));  // Example data for VectorValue
            std::string blob(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float));

            // Insert into Vector
            insertVector.bind(1, 1);  // versionId
            insertVector.bind(2, i);  // unique_id
            insertVector.bind(3, static_cast<int>(VectorValueType::Dense));  // type
            insertVector.bind(4, 0);  // deleted
            insertVector.exec();
            int vectorId = static_cast<int>(db.getLastInsertRowid());
            insertVector.reset();

            // Insert corresponding data into VectorValue
            insertVectorValue.bind(1, vectorId);  // vectorId
            insertVectorValue.bind(2, vectorIndexId);  // vectorIndexId
            insertVectorValue.bind(3, static_cast<int>(VectorValueType::Dense));  // type
            insertVectorValue.bind(4, blob);  // data
            insertVectorValue.exec();
            insertVectorValue.reset();
        }
    }

    std::string indexFileName;
    int vectorIndexId;
    int dim;
    int maxElements;
    std::unique_ptr<FaissIndexManager> indexManager;
};

// Test: Verify that vectors are correctly added to the FAISS index
TEST_F(FaissIndexManagerTest, TestDumpVectorsToIndex) {
    ASSERT_NO_THROW({
        indexManager->restoreVectorsToIndex();
    });

    // Verify that vectors were added to the index by performing a search
    ASSERT_NO_THROW({
        auto results = indexManager->search(std::vector<float>(dim, 0.0f), 5);
        ASSERT_EQ(results.size(), 5);
    });
}

// Test: Ensure that the index can be saved to and loaded from disk
TEST_F(FaissIndexManagerTest, TestSaveAndLoadIndex) {
    ASSERT_NO_THROW({
        indexManager->restoreVectorsToIndex();
        indexManager->saveIndex();
    });

    // Verify saving does not throw exceptions
    ASSERT_NO_THROW({
        indexManager->saveIndex();
    });

    // Create a new instance to load the index
    std::unique_ptr<FaissIndexManager> newIndexManager = std::make_unique<FaissIndexManager>(
        indexFileName,
        vectorIndexId,
        dim,
        maxElements,
        MetricType::L2,
        VectorValueType::Dense,
        HnswConfig(16, 200),
        QuantizationConfig()
    );

    // Load the index and perform a search
    ASSERT_NO_THROW({
        newIndexManager->loadIndex();
        auto results = newIndexManager->search(std::vector<float>(dim, 0.0f), 5);
        ASSERT_EQ(results.size(), 5);
    });
}

// Test: Validate that searching the index returns expected results
TEST_F(FaissIndexManagerTest, TestSearchFunctionality) {
    ASSERT_NO_THROW({
        indexManager->restoreVectorsToIndex();
    });

    // Perform a search with a query vector similar to one of the inserted vectors
    std::vector<float> queryVector(dim, 1.0f); // Similar to vectors with data [1,1,...,1]
    auto results = indexManager->search(queryVector, 3);

    ASSERT_EQ(results.size(), 3);

    // Since all vectors have distinct values, the closest should be the one with [1,1,...,1]
    // However, due to FAISS's internal workings, exact matching may vary
    // Here we check that the top result has the smallest distance
    // For a more robust test, compare the expected unique_id based on your mapping

    // Example assertion (assuming the closest vector has unique_id=1)
    EXPECT_EQ(results[0].second, 1);
}

// Additional Test: Add new vectors after initial index creation and verify search
TEST_F(FaissIndexManagerTest, TestAddVectorsAfterInitialization) {
    ASSERT_NO_THROW({
        indexManager->restoreVectorsToIndex();
    });

    // Add a new vector
    std::vector<float> newVector(dim, 500.0f);
    int newVectorId = 10; // Assuming unique_id starts at 0
    ASSERT_NO_THROW({
        indexManager->addVectorData(newVector, newVectorId);
    });

    // Perform a search to ensure the new vector is retrievable
    auto results = indexManager->search(newVector, 1);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].second, newVectorId);
}

TEST_F(FaissIndexManagerTest, TestAddAndSearch4DimVectors) {
    ASSERT_NO_THROW({
        indexManager->restoreVectorsToIndex();
    });

    // Define two new vectors with 16 dimensions, only first 4 non-zero
    std::vector<float> vectorA(dim, 0.0f);
    vectorA[0] = 1.0f;
    vectorA[1] = 2.0f;
    vectorA[2] = 3.0f;
    vectorA[3] = 4.0f;
    int vectorAId = 11;

    std::vector<float> vectorB(dim, 0.0f);
    vectorB[4] = 5.0f;
    vectorB[5] = 6.0f;
    vectorB[6] = 7.0f;
    vectorB[7] = 8.0f;
    int vectorBId = 12;

    // Add the vectors to the index
    ASSERT_NO_THROW({
        indexManager->addVectorData(vectorA, vectorAId);
        indexManager->addVectorData(vectorB, vectorBId);
    });

    // Perform a search for Vector A
    auto searchResultsA = indexManager->search(vectorA, 1);
    ASSERT_EQ(searchResultsA.size(), 1);
    EXPECT_EQ(searchResultsA[0].second, vectorAId);
    // Since the query matches exactly, distance should be near 0
    EXPECT_NEAR(searchResultsA[0].first, 0.0f, 1e-5);

    // Perform a search for Vector B
    auto searchResultsB = indexManager->search(vectorB, 1);
    ASSERT_EQ(searchResultsB.size(), 1);
    EXPECT_EQ(searchResultsB[0].second, vectorBId);
    // Since the query matches exactly, distance should be near 0
    EXPECT_NEAR(searchResultsB[0].first, 0.0f, 1e-5);

    // Create a query vector that is a combination of Vector A and Vector B
    // For example, set dimensions 0-3 to match Vector A and dimensions 4-7 to match Vector B
    std::vector<float> queryVector(dim, 0.0f);
    queryVector[0] = 1.0f;
    queryVector[1] = 2.0f;
    queryVector[2] = 3.0f;
    queryVector[3] = 4.0f;
    queryVector[4] = 5.0f;
    queryVector[5] = 6.0f;
    queryVector[6] = 7.0f;
    queryVector[7] = 8.0f;

    // Expected distance between queryVector and vectorA
    // Difference in dimensions 0-3: [0,0,0,0]
    // Difference in dimensions 4-7: [5, 6, 7, 8]
    float expectedDistanceToA = (25.0f + 36.0f + 49.0f + 64.0f);

    // Expected distance between queryVector and vectorB
    // Difference in dimensions 0-3: [1-0, 2-0, 3-0, 4-0] = [1,2,3,4]
    // Difference in dimensions 4-7: [0,0,0,0]
    // L2 distance = (1^2 + 2^2 + 3^2 + 4^2)
    float expectedDistanceToB = (1.0f + 4.0f + 9.0f + 16.0f);

    auto searchResultsCombined = indexManager->search(queryVector, 20);

    bool foundA = false;
    bool foundB = false;

    for (const auto& result : searchResultsCombined) {
        if (result.second == vectorAId) {
            foundA = true;
            EXPECT_NEAR(result.first, expectedDistanceToA, 1e-5);
        } else if (result.second == vectorBId) {
            foundB = true;
            EXPECT_NEAR(result.first, expectedDistanceToB, 1e-5);
        }
    }

    EXPECT_TRUE(foundA) << "Vector A was not found in the search results.";
    EXPECT_TRUE(foundB) << "Vector B was not found in the search results.";
}