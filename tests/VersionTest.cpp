#include <gtest/gtest.h>
#include <thread>
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "DatabaseManager.hpp"

#include "utils/Utils.hpp"

using namespace atinyvectors;
using namespace atinyvectors::utils;

// Fixture for VersionManager tests
class VersionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize managers
        SpaceManager::getInstance();
        VectorIndexManager::getInstance();
        VersionManager::getInstance();
        VectorManager::getInstance();
        DatabaseManager::getInstance();

        // Clear relevant tables to ensure a clean state before each test
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }

    void TearDown() override {
        // Clear relevant tables after each test to maintain isolation
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }
};

// Test for adding a new version and verifying time fields
TEST_F(VersionManagerTest, AddVersionWithAutomaticTime) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionManager& versionManager = VersionManager::getInstance();

    // Add a new space and capture the returned spaceId
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    int spaceId = spaceManager.addSpace(space); // Capture the returned id

    // Record current time before adding the version
    long beforeAdd = getCurrentTimeUTC();

    // Add the first version with is_default set to true
    Version version1;
    version1.spaceId = spaceId;
    version1.name = "Version 1.0";
    version1.description = "Initial version";
    version1.tag = "v1.0";
    version1.is_default = true;
    // Do not set created_time_utc and updated_time_utc manually

    int versionId1 = versionManager.addVersion(version1);

    EXPECT_EQ(version1.id, versionId1);
    EXPECT_TRUE(version1.is_default);

    // Retrieve the version and verify fields
    Version retrieved = versionManager.getVersionById(versionId1);

    EXPECT_EQ(retrieved.id, versionId1);
    EXPECT_EQ(retrieved.spaceId, spaceId);
    EXPECT_EQ(retrieved.unique_id, 1); // first unique id should be 1
    EXPECT_EQ(retrieved.name, "Version 1.0");
    EXPECT_EQ(retrieved.description, "Initial version");
    EXPECT_EQ(retrieved.tag, "v1.0");
    EXPECT_TRUE(retrieved.is_default);
}

// Test for updating a version and verifying updated_time_utc
TEST_F(VersionManagerTest, UpdateVersionWithAutomaticTime) {
    VersionManager& versionManager = VersionManager::getInstance();
    SpaceManager& spaceManager = SpaceManager::getInstance();

    // Add a new space and capture the returned spaceId
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    int spaceId = spaceManager.addSpace(space); // Capture the returned id

    // Add a version
    Version version;
    version.spaceId = spaceId;
    version.name = "Version 1.0";
    version.description = "Initial version";
    version.tag = "v1.0";
    version.is_default = false;

    int versionId = versionManager.addVersion(version);
    EXPECT_GT(versionId, 0);

    // Retrieve the original version
    Version original = versionManager.getVersionById(versionId);
    long original_created = original.created_time_utc;
    long original_updated = original.updated_time_utc;

    // Wait for 2 seconds to ensure updated_time_utc will be different
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Update the version
    Version updatedVersion = original;
    updatedVersion.description = "Updated description";
    updatedVersion.tag = "v1.1";
    updatedVersion.is_default = true;

    versionManager.updateVersion(updatedVersion);

    // Retrieve the updated version
    Version afterUpdate = versionManager.getVersionById(versionId);

    EXPECT_EQ(afterUpdate.description, "Updated description");
    EXPECT_EQ(afterUpdate.tag, "v1.1");
    EXPECT_TRUE(afterUpdate.is_default);

    // created_time_utc should not change
    EXPECT_EQ(afterUpdate.created_time_utc, original_created);

    // updated_time_utc should be greater than original
    EXPECT_GT(afterUpdate.updated_time_utc, original_updated);
}

// Test for getting a version by unique_id
TEST_F(VersionManagerTest, GetVersionByUniqueId) {
    VersionManager& versionManager = VersionManager::getInstance();
    SpaceManager& spaceManager = SpaceManager::getInstance();

    // Add a new space and capture the returned spaceId
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    int spaceId = spaceManager.addSpace(space); // Capture the returned id

    // Add two versions to the same space
    Version version1;
    version1.spaceId = spaceId;
    version1.name = "Version 1.0";
    version1.description = "First version";
    version1.tag = "v1.0";
    version1.is_default = false;
    int versionId1 = versionManager.addVersion(version1);
    EXPECT_GT(versionId1, 0);

    Version version2;
    version2.spaceId = spaceId;
    version2.name = "Version 2.0";
    version2.description = "Second version";
    version2.tag = "v2.0";
    version2.is_default = true;
    int versionId2 = versionManager.addVersion(version2);
    EXPECT_GT(versionId2, 0);

    // Retrieve versions by unique_id
    Version retrieved1 = versionManager.getVersionByUniqueId(spaceId, 1);
    EXPECT_EQ(retrieved1.id, versionId1);
    EXPECT_EQ(retrieved1.unique_id, 1);
    EXPECT_EQ(retrieved1.name, "Version 1.0");
    EXPECT_FALSE(retrieved1.is_default);

    Version retrieved2 = versionManager.getVersionByUniqueId(spaceId, 2);
    EXPECT_EQ(retrieved2.id, versionId2);
    EXPECT_EQ(retrieved2.unique_id, 2);
    EXPECT_EQ(retrieved2.name, "Version 2.0");
    EXPECT_TRUE(retrieved2.is_default);

    // Attempt to retrieve a non-existent unique_id
    EXPECT_THROW(versionManager.getVersionByUniqueId(spaceId, 3), std::runtime_error);
}

// Test for adding multiple versions and verifying unique_id increment
TEST_F(VersionManagerTest, UniqueIdIncrement) {
    VersionManager& versionManager = VersionManager::getInstance();
    SpaceManager& spaceManager = SpaceManager::getInstance();

    // Add a new space and capture the returned spaceId
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    int spaceId = spaceManager.addSpace(space); // Capture the returned id

    // Add first version
    Version version1;
    version1.spaceId = spaceId;
    version1.name = "Version 1.0";
    version1.description = "First version";
    version1.tag = "v1.0";
    version1.is_default = false;
    int versionId1 = versionManager.addVersion(version1);
    EXPECT_EQ(version1.unique_id, 1);

    // Add second version
    Version version2;
    version2.spaceId = spaceId;
    version2.name = "Version 2.0";
    version2.description = "Second version";
    version2.tag = "v2.0";
    version2.is_default = false;
    int versionId2 = versionManager.addVersion(version2);
    EXPECT_EQ(version2.unique_id, 2);

    // Add third version
    Version version3;
    version3.spaceId = spaceId;
    version3.name = "Version 3.0";
    version3.description = "Third version";
    version3.tag = "v3.0";
    version3.is_default = false;
    int versionId3 = versionManager.addVersion(version3);
    EXPECT_EQ(version3.unique_id, 3);
}

// Test for getting all versions
TEST_F(VersionManagerTest, GetAllVersions) {
    VersionManager& versionManager = VersionManager::getInstance();
    SpaceManager& spaceManager = SpaceManager::getInstance();

    // Add a new space and capture the returned spaceId
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    int spaceId = spaceManager.addSpace(space); // Capture the returned id

    // Add multiple versions
    Version version1;
    version1.spaceId = spaceId;
    version1.name = "Version 1.0";
    version1.description = "First version";
    version1.tag = "v1.0";
    version1.is_default = false;
    versionManager.addVersion(version1);

    Version version2;
    version2.spaceId = spaceId;
    version2.name = "Version 2.0";
    version2.description = "Second version";
    version2.tag = "v2.0";
    version2.is_default = true;
    versionManager.addVersion(version2);

    // Retrieve all versions
    auto versions = versionManager.getAllVersions();
    ASSERT_EQ(versions.size(), 2);

    // Verify each version
    EXPECT_EQ(versions[0].name, "Version 1.0");
    EXPECT_FALSE(versions[0].is_default);

    EXPECT_EQ(versions[1].name, "Version 2.0");
    EXPECT_TRUE(versions[1].is_default);
}

// Test for deleting a version and verifying default version reassignment
TEST_F(VersionManagerTest, DeleteVersionAndReassignDefault) {
    VersionManager& versionManager = VersionManager::getInstance();
    SpaceManager& spaceManager = SpaceManager::getInstance();

    // Add a new space and capture the returned spaceId
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    int spaceId = spaceManager.addSpace(space); // Capture the returned id

    // Add two versions, second being default
    Version version1;
    version1.spaceId = spaceId;
    version1.name = "Version 1.0";
    version1.description = "First version";
    version1.tag = "v1.0";
    version1.is_default = false;
    int versionId1 = versionManager.addVersion(version1);

    Version version2;
    version2.spaceId = spaceId;
    version2.name = "Version 2.0";
    version2.description = "Second version";
    version2.tag = "v2.0";
    version2.is_default = true;
    int versionId2 = versionManager.addVersion(version2);

    // Verify initial default
    auto versions = versionManager.getVersionsBySpaceId(spaceId);
    ASSERT_EQ(versions.size(), 2);
    EXPECT_FALSE(versions[0].is_default);
    EXPECT_TRUE(versions[1].is_default);

    // Delete the default version (version2)
    versionManager.deleteVersion(versionId2);

    // Retrieve remaining versions
    versions = versionManager.getVersionsBySpaceId(spaceId);
    ASSERT_EQ(versions.size(), 1);
    EXPECT_EQ(versions[0].id, versionId1);
    EXPECT_TRUE(versions[0].is_default); // version1 should now be default
}

// Test for handling a non-existent version
TEST_F(VersionManagerTest, HandleNonExistentVersion) {
    VersionManager& manager = VersionManager::getInstance();

    // Trying to get a version with an ID that does not exist
    EXPECT_THROW(manager.getVersionById(999), std::runtime_error);

    // Trying to get a version by unique_id that does not exist
    EXPECT_THROW(manager.getVersionByUniqueId(1, 999), std::runtime_error);
}