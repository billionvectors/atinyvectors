#include "algo/HnswIndexManager.hpp"
#include "DatabaseManager.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorIndexOptimizer.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Config.hpp"
#include "gtest/gtest.h"

using namespace atinyvectors;
using namespace atinyvectors::algo;

// Test Fixture for HnswIndexManager
class HnswIndexManagerTest : public ::testing::Test {
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

        // Setup code here: Initialize the database and create mock data
        indexFileName = "test_hnsw_index.bin";
        dim = 16;
        maxElements = 1000;

        createMockDatas();

        // Create the index manager with a specific MetricType (e.g., L2)
        indexManager = std::make_unique<HnswIndexManager>(indexFileName, dim, maxElements, MetricType::L2);
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

    void createMockDatas() {
        auto& db = DatabaseManager::getInstance().getDatabase();

        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM VectorIndexOptimizer;");

        db.exec("INSERT INTO VectorIndexOptimizer (vectorIndexId, hnswConfigJson, quantizationConfigJson) VALUES (1, '{\"M\": 16, \"EfConstruct\": 200}', '{}')");

        SQLite::Statement insertVector(db, "INSERT INTO Vector (versionId, unique_id, type, deleted) VALUES (?, ?, ?, ?)");
        SQLite::Statement insertVectorValue(db, "INSERT INTO VectorValue (vectorId, vectorIndexId, type, data) VALUES (?, ?, ?, ?)");

        for (int i = 0; i < 10; ++i) {
            std::vector<float> data(16, static_cast<float>(i));  // Example data for VectorValue
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
            insertVectorValue.bind(2, 1);  // vectorIndexId
            insertVectorValue.bind(3, static_cast<int>(VectorValueType::Dense));  // type
            insertVectorValue.bind(4, blob);  // data
            insertVectorValue.exec();
            insertVectorValue.reset();
        }
    }

    std::string indexFileName;
    int dim;
    int maxElements;
    std::unique_ptr<HnswIndexManager> indexManager;
};

TEST_F(HnswIndexManagerTest, TestDumpVectorsToIndex) {
    indexManager->dumpVectorsToIndex(1);

    // Verify that vectors were added to the index
    ASSERT_NO_THROW({
        indexManager->search(std::vector<float>(16, 0.0f), 5);
    });
}

TEST_F(HnswIndexManagerTest, TestSaveAndLoadIndex) {
    indexManager->dumpVectorsToIndex(1);
    indexManager->saveIndex();

    // Verify saving does not throw exceptions
    ASSERT_NO_THROW({
        indexManager->saveIndex();
    });

    // Reload and verify search
    ASSERT_NO_THROW({
        indexManager->loadIndex();
        auto results = indexManager->search(std::vector<float>(16, 0.0f), 5);
        ASSERT_EQ(results.size(), 5);
    });
}

TEST_F(HnswIndexManagerTest, TestSearchFunctionality) {
    indexManager->dumpVectorsToIndex(1);

    auto results = indexManager->search(std::vector<float>(16, 1.0f), 3);
    ASSERT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].second, 1);
}   
