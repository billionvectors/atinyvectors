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

// Fixture for VectorMetadataManager tests
class VectorMetadataManagerTest : public ::testing::Test {
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

        HnswConfig hnswConfig(16, 200);  // Example HNSW config: M = 16, EfConstruct = 200
        QuantizationConfig quantizationConfig;  // Default empty quantization config

        VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", MetricType::L2, 4,
                                hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 0, 0, true);
        indexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);
    }

    int spaceId;
    int versionId;
    int indexId;
};

// Test for adding new VectorMetadata
TEST_F(VectorMetadataManagerTest, AddVectorMetadata) {
    VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();

    VectorMetadata metadata(0, 1001, "Key1", "Value1");
    int metadataId = metadataManager.addVectorMetadata(metadata);

    EXPECT_EQ(metadata.id, metadataId);

    auto metadataList = metadataManager.getVectorMetadataByVectorId(1001);
    ASSERT_EQ(metadataList.size(), 1);
    EXPECT_EQ(metadataList[0].id, metadataId);
    EXPECT_EQ(metadataList[0].key, "Key1");
    EXPECT_EQ(metadataList[0].value, "Value1");
}

// Test for getting VectorMetadata by ID
TEST_F(VectorMetadataManagerTest, GetVectorMetadataById) {
    VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();

    VectorMetadata metadata(0, 1001, "Key1", "Value1");
    metadataManager.addVectorMetadata(metadata);

    auto metadataList = metadataManager.getAllVectorMetadata();
    ASSERT_FALSE(metadataList.empty());

    int metadataId = metadataList[0].id;
    VectorMetadata retrievedMetadata = metadataManager.getVectorMetadataById(metadataId);

    EXPECT_EQ(retrievedMetadata.id, metadataId);
    EXPECT_EQ(retrievedMetadata.key, "Key1");
    EXPECT_EQ(retrievedMetadata.value, "Value1");
}

// Test for getting VectorMetadata by vector ID
TEST_F(VectorMetadataManagerTest, GetVectorMetadataByVectorId) {
    VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();

    // Adding multiple metadata to the same vector
    VectorMetadata metadata1(0, 1001, "Key1", "Value1");
    VectorMetadata metadata2(0, 1001, "Key2", "Value2");
    metadataManager.addVectorMetadata(metadata1);
    metadataManager.addVectorMetadata(metadata2);

    auto metadataList = metadataManager.getVectorMetadataByVectorId(1001);
    ASSERT_EQ(metadataList.size(), 2);
    EXPECT_EQ(metadataList[0].key, "Key1");
    EXPECT_EQ(metadataList[1].key, "Key2");
}

// Test for updating VectorMetadata
TEST_F(VectorMetadataManagerTest, UpdateVectorMetadata) {
    VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();

    VectorMetadata metadata(0, 1001, "Key1", "Value1");
    metadataManager.addVectorMetadata(metadata);

    auto metadataList = metadataManager.getAllVectorMetadata();
    ASSERT_FALSE(metadataList.empty());

    int metadataId = metadataList[0].id;
    VectorMetadata updatedMetadata = metadataList[0];
    updatedMetadata.value = "Updated Value";

    metadataManager.updateVectorMetadata(updatedMetadata);

    metadataList = metadataManager.getVectorMetadataByVectorId(1001);
    ASSERT_EQ(metadataList.size(), 1);
    EXPECT_EQ(metadataList[0].id, metadataId);
    EXPECT_EQ(metadataList[0].value, "Updated Value");
}

// Test for deleting VectorMetadata
TEST_F(VectorMetadataManagerTest, DeleteVectorMetadata) {
    VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();

    // Add two metadata entries
    VectorMetadata metadata1(0, 1001, "Key1", "Value1");
    VectorMetadata metadata2(0, 1001, "Key2", "Value2");
    metadataManager.addVectorMetadata(metadata1);
    metadataManager.addVectorMetadata(metadata2);

    // Delete the second metadata entry
    metadataManager.deleteVectorMetadata(metadata2.id);

    auto metadataList = metadataManager.getVectorMetadataByVectorId(1001);
    ASSERT_EQ(metadataList.size(), 1);
    EXPECT_EQ(metadataList[0].id, metadata1.id);
}

// Test for handling a non-existent VectorMetadata
TEST_F(VectorMetadataManagerTest, HandleNonExistentVectorMetadata) {
    VectorMetadataManager& manager = VectorMetadataManager::getInstance();

    // Trying to get metadata with an ID that does not exist
    EXPECT_THROW(manager.getVectorMetadataById(999), std::runtime_error);
}

// Test for filtering vectors
TEST_F(VectorMetadataManagerTest, FilterVectors) {
    VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();
    VectorManager& vectorManager = VectorManager::getInstance();
    
    // Setup versionId for vectors
    int versionId = 1; // Assume a version ID for the test, adjust as needed.

    // Adding vectors to the Vector table to ensure vector data exists
    Vector vector1(1001, versionId, 1001, VectorValueType::Dense, {}, false);
    Vector vector2(1002, versionId, 1002, VectorValueType::Dense, {}, false);
    Vector vector3(1003, versionId, 1003, VectorValueType::Dense, {}, false);
    
    vectorManager.addVector(vector1);
    vectorManager.addVector(vector2);
    vectorManager.addVector(vector3);

    // Adding metadata entries for different vector IDs
    VectorMetadata metadata1(0, vector1.id, "status", "active");
    VectorMetadata metadata2(0, vector2.id, "status", "inactive");
    VectorMetadata metadata3(0, vector3.id, "status", "active");
    metadataManager.addVectorMetadata(metadata1);
    metadataManager.addVectorMetadata(metadata2);
    metadataManager.addVectorMetadata(metadata3);

    // Prepare input vectors with unique_id values
    std::vector<std::pair<float, hnswlib::labeltype>> inputVectors = {
        {0.1f, 1001},  // unique_id
        {0.2f, 1002},  // unique_id
        {0.3f, 1003}   // unique_id
    };

    // Filtering for "active" status
    std::string filter = "status == 'active'";
    auto filteredVectors = metadataManager.filterVectors(inputVectors, filter);

    // Check if the result contains only active vectors
    ASSERT_EQ(filteredVectors.size(), 2);
    EXPECT_EQ(filteredVectors[0].second, 1001);
    EXPECT_EQ(filteredVectors[1].second, 1003);

    // Filtering for "inactive" status
    filter = "status == 'inactive'";
    filteredVectors = metadataManager.filterVectors(inputVectors, filter);

    // Check if the result contains only inactive vectors
    ASSERT_EQ(filteredVectors.size(), 1);
    EXPECT_EQ(filteredVectors[0].second, 1002);
}
