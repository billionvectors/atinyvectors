#include <gtest/gtest.h>
#include "algo/HnswIndexLRUCache.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Snapshot.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "utils/Utils.hpp"

using namespace atinyvectors;
using namespace atinyvectors::algo;
using namespace atinyvectors::utils;

// Fixture for VectorManager tests
class VectorManagerTest : public ::testing::Test {
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

        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");

        // clean data
        HnswIndexLRUCache::getInstance().clean();

        createTestIndex();
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
    }

    void createTestIndex() {
        SpaceManager& spaceManager = SpaceManager::getInstance();
        VersionManager& versionManager = VersionManager::getInstance();

        // Add a new space and capture the returned spaceId
        Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
        spaceId = spaceManager.addSpace(space); // Capture the returned id

        // Add the first version with is_default set to true
        Version version1;
        version1.spaceId = spaceId;
        version1.name = "Version 1.0";
        version1.description = "Initial version";
        version1.tag = "v1.0";
        version1.is_default = true;

        versionId = versionManager.addVersion(version1);
    }

    int spaceId;
    int versionId;
};

// Test for adding a new vector
TEST_F(VectorManagerTest, AddVector) {
    VectorManager& manager = VectorManager::getInstance();
    
    // Update constructor call to include unique_id
    Vector vector(0, versionId, 0, VectorValueType::Dense, {}, false); // Provide 0 as unique_id
    int vectorId = manager.addVector(vector);

    EXPECT_EQ(vector.id, vectorId);  // Ensure ID is set correctly

    auto vectors = manager.getAllVectors();
    ASSERT_EQ(vectors.size(), 1);
    EXPECT_EQ(vectors[0].id, vectorId);
    EXPECT_EQ(vectors[0].versionId, versionId);
    EXPECT_EQ(vectors[0].type, VectorValueType::Dense);
    EXPECT_FALSE(vectors[0].deleted);
}

// Test for getting a vector by ID
TEST_F(VectorManagerTest, GetVectorById) {
    VectorManager& manager = VectorManager::getInstance();

    // Update constructor call to include unique_id
    Vector vector(0, versionId, 0, VectorValueType::Sparse, {}, false); // Provide 0 as unique_id
    manager.addVector(vector);

    auto vectors = manager.getAllVectors();
    ASSERT_FALSE(vectors.empty());

    unsigned long long vectorId = vectors[0].id;
    Vector retrievedVector = manager.getVectorById(vectorId);

    EXPECT_EQ(retrievedVector.id, vectorId);
    EXPECT_EQ(retrievedVector.versionId, versionId);
    EXPECT_EQ(retrievedVector.type, VectorValueType::Sparse);
}

// Test for updating a vector
TEST_F(VectorManagerTest, UpdateVector) {
    VectorManager& manager = VectorManager::getInstance();

    // Update constructor call to include unique_id
    Vector vector(0, versionId, 0, VectorValueType::Combined, {}, false); // Provide 0 as unique_id
    manager.addVector(vector);

    auto vectors = manager.getAllVectors();
    ASSERT_FALSE(vectors.empty());

    unsigned long long vectorId = vectors[0].id;
    Vector updatedVector = vectors[0];
    updatedVector.type = VectorValueType::MultiVector;
    manager.updateVector(updatedVector);

    Vector retrievedVector = manager.getVectorById(vectorId);
    EXPECT_EQ(retrievedVector.type, VectorValueType::MultiVector);
}

// Test for deleting a vector
TEST_F(VectorManagerTest, DeleteVector) {
    VectorManager& manager = VectorManager::getInstance();

    // Update constructor call to include unique_id
    Vector vector(0, versionId, 0, VectorValueType::Dense, {}, false); // Provide 0 as unique_id
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

    auto& db = DatabaseManager::getInstance().getDatabase();
    
    // Insert a VectorIndex entry
    SQLite::Statement indexQuery(db, "INSERT INTO VectorIndex (versionId, vectorValueType, name, metricType, dimension, hnswConfigJson, quantizationConfigJson, create_date_utc, updated_time_utc, is_default) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    indexQuery.bind(1, versionId);  // versionId
    indexQuery.bind(2, static_cast<int>(VectorValueType::Dense));  // vectorValueType
    indexQuery.bind(3, "TestIndex");  // name
    indexQuery.bind(4, static_cast<int>(MetricType::L2));  // metricType
    indexQuery.bind(5, 128);  // dimension
    indexQuery.bind(6, "{\"M\": 16, \"EfConstruct\": 200}");  // hnswConfigJson
    indexQuery.bind(7, "{}");  // quantizationConfigJson
    indexQuery.bind(8, 123456789);  // create_date_utc
    indexQuery.bind(9, 123456789);  // updated_time_utc
    indexQuery.bind(10, 1);  // is_default
    indexQuery.exec();
    
    int vectorIndexId = static_cast<int>(db.getLastInsertRowid());

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
