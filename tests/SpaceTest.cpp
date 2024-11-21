#include <gtest/gtest.h>
#include "algo/FaissIndexLRUCache.hpp"
#include "utils/Utils.hpp"
#include "Snapshot.hpp"
#include "Space.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "Version.hpp"
#include "VectorMetadata.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "RbacToken.hpp"
#include "DatabaseManager.hpp"

using namespace atinyvectors;
using namespace atinyvectors::algo;
using namespace atinyvectors::utils;

// Fixture for SpaceManager tests
class SpaceManagerTest : public ::testing::Test {
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

        RbacTokenManager& rbacTokenManager = RbacTokenManager::getInstance();
        rbacTokenManager.createTable();

        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM RbacToken;");

        // clean data
        FaissIndexLRUCache::getInstance().clean();
    }

    void TearDown() override {
        // Clear data after each test
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        
        try {
            db.exec("DELETE FROM Vector;"); // we will drop vector table in "DeleteSpaceWithoutVectorTable"
        } catch (const SQLite::Exception& e) {
        }

        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM RbacToken;");
    }
};

// Test for adding a new space
TEST_F(SpaceManagerTest, AddSpace) {
    SpaceManager& manager = SpaceManager::getInstance();
    
    Space space(0, "Test Space", "A space for testing", 1627906032, 1627906032);
    int spaceId = manager.addSpace(space);

    EXPECT_EQ(space.id, spaceId);  // Ensure ID is set correctly

    auto spaces = manager.getAllSpaces();
    ASSERT_EQ(spaces.size(), 1);
    EXPECT_EQ(spaces[0].id, spaceId);
    EXPECT_EQ(spaces[0].name, "Test Space");
    EXPECT_EQ(spaces[0].description, "A space for testing");
}

// Test for getting a space by ID
TEST_F(SpaceManagerTest, GetSpaceById) {
    SpaceManager& manager = SpaceManager::getInstance();

    Space space(0, "Test Space", "A space for testing", 1627906032, 1627906032);
    manager.addSpace(space);

    auto spaces = manager.getAllSpaces();
    ASSERT_FALSE(spaces.empty());

    int spaceId = spaces[0].id;
    Space retrievedSpace = manager.getSpaceById(spaceId);

    EXPECT_EQ(retrievedSpace.id, spaceId);
    EXPECT_EQ(retrievedSpace.name, "Test Space");
    EXPECT_EQ(retrievedSpace.description, "A space for testing");
}

// Test for updating a space
TEST_F(SpaceManagerTest, UpdateSpace) {
    SpaceManager& manager = SpaceManager::getInstance();

    Space space(0, "Test Space", "A space for testing", 1627906032, 1627906032);
    manager.addSpace(space);

    auto spaces = manager.getAllSpaces();
    ASSERT_FALSE(spaces.empty());

    int spaceId = spaces[0].id;
    Space updatedSpace = spaces[0];
    updatedSpace.name = "Updated Test Space";
    manager.updateSpace(updatedSpace);

    Space retrievedSpace = manager.getSpaceById(spaceId);
    EXPECT_EQ(retrievedSpace.name, "Updated Test Space");
}

// Test for handling a non-existent space
TEST_F(SpaceManagerTest, HandleNonExistentSpace) {
    SpaceManager& manager = SpaceManager::getInstance();

    // Trying to get a space with an ID that does not exist
    EXPECT_THROW(manager.getSpaceById(999), std::runtime_error);
}

// Test for deleting a space when Vector table is missing
TEST_F(SpaceManagerTest, DeleteSpaceWithoutVectorTable) {
    SpaceManager& manager = SpaceManager::getInstance();
    auto& db = DatabaseManager::getInstance().getDatabase();

    // 1. Add a test space
    Space space(0, "Test Space", "A space for testing", 1627906032, 1627906032);
    int spaceId = manager.addSpace(space);

    // 2. Simulate Vector table deletion
    db.exec("DROP TABLE IF EXISTS Vector;");

    // 3. Attempt to delete the space
    EXPECT_NO_THROW({
        manager.deleteSpace(spaceId);
    });

    // 4. Verify that the space is deleted
    auto spaces = manager.getAllSpaces();
    EXPECT_TRUE(spaces.empty());
}
