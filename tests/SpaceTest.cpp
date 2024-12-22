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

        // clean data
        DatabaseManager::getInstance().reset();
        FaissIndexLRUCache::getInstance().clean();
    }

    void TearDown() override {
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
