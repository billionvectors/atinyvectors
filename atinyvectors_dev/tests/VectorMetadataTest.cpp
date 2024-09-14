#include <gtest/gtest.h>
#include "VectorMetadata.hpp"
#include "DatabaseManager.hpp"

using namespace atinyvectors;

// Fixture for VectorMetadataManager tests
class VectorMetadataManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up a clean test database
        VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM VectorMetadata;");
    }

    void TearDown() override {
        // Cleanup after each test if necessary
        VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM VectorMetadata;");
    }
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

