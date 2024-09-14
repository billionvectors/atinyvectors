#include <gtest/gtest.h>
#include "Version.hpp"
#include "VectorIndex.hpp"
#include "VectorIndexOptimizer.hpp"
#include "DatabaseManager.hpp"
#include "ValueType.hpp"
#include "Vector.hpp"

using namespace atinyvectors;

// Fixture for VectorManager tests
class VectorManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up a clean test database
        VectorManager& vectorManager = VectorManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();

        createTestIndexAndOptimizer();
    }

    void TearDown() override {
        // Cleanup after each test if necessary
        VectorManager& vectorManager = VectorManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM VectorIndexOptimizer;");
    }

    // Test용 인덱스와 최적화 데이터를 생성하는 함수
    void createTestIndexAndOptimizer() {
        auto& db = DatabaseManager::getInstance().getDatabase();

        VectorIndexManager& vectorIndexManager = VectorIndexManager::getInstance();
        VectorIndexOptimizerManager& vectorIndexOptimizerManager = VectorIndexOptimizerManager::getInstance();
        VersionManager& versionManager = VersionManager::getInstance();
        
        // VectorIndex 생성
        SQLite::Statement indexQuery(db, "INSERT INTO VectorIndex (versionId, vectorValueType, name, create_date_utc, updated_time_utc, is_default) VALUES (?, ?, ?, ?, ?, ?)");
        indexQuery.bind(1, 1); // versionId
        indexQuery.bind(2, static_cast<int>(VectorValueType::Dense)); // vectorValueType
        indexQuery.bind(3, "TestIndex"); // name
        indexQuery.bind(4, 123456789); // create_date_utc
        indexQuery.bind(5, 123456789); // updated_time_utc
        indexQuery.bind(6, 1); // is_default
        indexQuery.exec();

        int vectorIndexId = static_cast<int>(db.getLastInsertRowid());

        // VectorIndexOptimizer 생성
        SQLite::Statement optimizerQuery(db, "INSERT INTO VectorIndexOptimizer (vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson) VALUES (?, ?, ?, ?, ?)");
        optimizerQuery.bind(1, vectorIndexId); // vectorIndexId
        optimizerQuery.bind(2, static_cast<int>(MetricType::L2)); // metricType
        optimizerQuery.bind(3, 128); // dimension
        optimizerQuery.bind(4, "{\"M\": 16, \"EfConstruct\": 200}"); // hnswConfigJson
        optimizerQuery.bind(5, "{}"); // quantizationConfigJson
        optimizerQuery.exec();
    }
};

// Test for adding a new vector
TEST_F(VectorManagerTest, AddVector) {
    VectorManager& manager = VectorManager::getInstance();
    
    // Update constructor call to include unique_id
    Vector vector(0, 1, 0, VectorValueType::Dense, {}, false); // Provide 0 as unique_id
    int vectorId = manager.addVector(vector);

    EXPECT_EQ(vector.id, vectorId);  // Ensure ID is set correctly

    auto vectors = manager.getAllVectors();
    ASSERT_EQ(vectors.size(), 1);
    EXPECT_EQ(vectors[0].id, vectorId);
    EXPECT_EQ(vectors[0].versionId, 1);
    EXPECT_EQ(vectors[0].type, VectorValueType::Dense);
    EXPECT_FALSE(vectors[0].deleted);
}

// Test for getting a vector by ID
TEST_F(VectorManagerTest, GetVectorById) {
    VectorManager& manager = VectorManager::getInstance();

    // Update constructor call to include unique_id
    Vector vector(0, 1, 0, VectorValueType::Sparse, {}, false); // Provide 0 as unique_id
    manager.addVector(vector);

    auto vectors = manager.getAllVectors();
    ASSERT_FALSE(vectors.empty());

    unsigned long long vectorId = vectors[0].id;
    Vector retrievedVector = manager.getVectorById(vectorId);

    EXPECT_EQ(retrievedVector.id, vectorId);
    EXPECT_EQ(retrievedVector.versionId, 1);
    EXPECT_EQ(retrievedVector.type, VectorValueType::Sparse);
}

// Test for updating a vector
TEST_F(VectorManagerTest, UpdateVector) {
    VectorManager& manager = VectorManager::getInstance();

    // Update constructor call to include unique_id
    Vector vector(0, 1, 0, VectorValueType::Combined, {}, false); // Provide 0 as unique_id
    manager.addVector(vector);

    auto vectors = manager.getAllVectors();
    ASSERT_FALSE(vectors.empty());

    unsigned long long vectorId = vectors[0].id;
    Vector updatedVector = vectors[0];
    updatedVector.versionId = 2;
    updatedVector.type = VectorValueType::MultiVector;
    manager.updateVector(updatedVector);

    Vector retrievedVector = manager.getVectorById(vectorId);
    EXPECT_EQ(retrievedVector.versionId, 2);
    EXPECT_EQ(retrievedVector.type, VectorValueType::MultiVector);
}

// Test for deleting a vector
TEST_F(VectorManagerTest, DeleteVector) {
    VectorManager& manager = VectorManager::getInstance();

    // Update constructor call to include unique_id
    Vector vector(0, 1, 0, VectorValueType::Dense, {}, false); // Provide 0 as unique_id
    manager.addVector(vector);

    auto vectors = manager.getAllVectors();
    ASSERT_FALSE(vectors.empty());

    unsigned long long vectorId = vectors[0].id;
    manager.deleteVector(vectorId);

    vectors = manager.getAllVectors();
    EXPECT_TRUE(vectors.empty());
}

// Test for handling a non-existent vector
TEST_F(VectorManagerTest, HandleNonExistentVector) {
    VectorManager& manager = VectorManager::getInstance();

    // Trying to get a vector with an ID that does not exist
    EXPECT_THROW(manager.getVectorById(999), std::runtime_error);
}

// Test for adding a vector with VectorValues
TEST_F(VectorManagerTest, AddVectorWithValues) {
    VectorManager& manager = VectorManager::getInstance();

    // Create a VectorIndex and VectorIndexOptimizer for the test
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    // Insert a VectorIndex entry
    SQLite::Statement indexQuery(db, "INSERT INTO VectorIndex (versionId, vectorValueType, name, create_date_utc, updated_time_utc, is_default) VALUES (?, ?, ?, ?, ?, ?)");
    indexQuery.bind(1, 1);  // versionId
    indexQuery.bind(2, static_cast<int>(VectorValueType::Dense));  // vectorValueType
    indexQuery.bind(3, "TestIndex");  // name
    indexQuery.bind(4, 123456789);  // create_date_utc
    indexQuery.bind(5, 123456789);  // updated_time_utc
    indexQuery.bind(6, 1);  // is_default
    indexQuery.exec();
    
    int vectorIndexId = static_cast<int>(db.getLastInsertRowid());

    // Insert a VectorIndexOptimizer entry with matching vectorIndexId
    SQLite::Statement optimizerQuery(db, "INSERT INTO VectorIndexOptimizer (vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson) VALUES (?, ?, ?, ?, ?)");
    optimizerQuery.bind(1, vectorIndexId);  // vectorIndexId
    optimizerQuery.bind(2, static_cast<int>(MetricType::L2));  // metricType
    optimizerQuery.bind(3, 128);  // dimension
    optimizerQuery.bind(4, "{\"M\": 16, \"EfConstruct\": 200}");  // hnswConfigJson
    optimizerQuery.bind(5, "{}");  // quantizationConfigJson
    optimizerQuery.exec();

    // Create a VectorValue with example data and the correct vectorIndexId
    VectorValue vectorValue(0, 0, vectorIndexId, VectorValueType::Dense, std::vector<float>{1.1f, 2.2f, 3.3f});

    // Update constructor call to include unique_id and vectorIndexId
    Vector vector(0, 1, 0, VectorValueType::Dense, {vectorValue}, false);  // Provide 0 as unique_id
    int vectorId = manager.addVector(vector);

    auto vectors = manager.getAllVectors();
    ASSERT_EQ(vectors.size(), 1);

    auto retrievedVector = manager.getVectorById(vectorId);
    ASSERT_EQ(retrievedVector.values.size(), 1);
    EXPECT_EQ(retrievedVector.values[0].vectorIndexId, vectorIndexId);

    const std::vector<float>& retrievedData = retrievedVector.values[0].denseData;

    ASSERT_EQ(retrievedData.size(), 3);
    EXPECT_FLOAT_EQ(retrievedData[0], 1.1f);
    EXPECT_FLOAT_EQ(retrievedData[1], 2.2f);
    EXPECT_FLOAT_EQ(retrievedData[2], 3.3f);
}
