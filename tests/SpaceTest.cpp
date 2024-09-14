#include <gtest/gtest.h>
#include "Space.hpp"
#include "DatabaseManager.hpp"

using namespace atinyvectors;

// Fixture for SpaceManager tests
class SpaceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up a clean test database
        SpaceManager& spaceManager = SpaceManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
    }

    void TearDown() override {
        // Cleanup after each test if necessary
        SpaceManager& spaceManager = SpaceManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
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

// Test for deleting a space
TEST_F(SpaceManagerTest, DeleteSpace) {
    SpaceManager& manager = SpaceManager::getInstance();

    Space space(0, "Test Space", "A space for testing", 1627906032, 1627906032);
    manager.addSpace(space);

    auto spaces = manager.getAllSpaces();
    ASSERT_FALSE(spaces.empty());

    int spaceId = spaces[0].id;
    manager.deleteSpace(spaceId);

    spaces = manager.getAllSpaces();
    EXPECT_TRUE(spaces.empty());
}

// Test for handling a non-existent space
TEST_F(SpaceManagerTest, HandleNonExistentSpace) {
    SpaceManager& manager = SpaceManager::getInstance();

    // Trying to get a space with an ID that does not exist
    EXPECT_THROW(manager.getSpaceById(999), std::runtime_error);
}