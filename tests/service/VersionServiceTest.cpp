#include <gtest/gtest.h>
#include "service/VersionService.hpp"
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
#include "nlohmann/json.hpp"

using namespace atinyvectors;
using namespace atinyvectors::service;
using namespace atinyvectors::algo;
using namespace atinyvectors::utils;
using namespace nlohmann;

// Fixture for VersionServiceManager tests
class VersionServiceManagerTest : public ::testing::Test {
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

// Test for creating a version with a given space name
TEST_F(VersionServiceManagerTest, CreateVersion) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionServiceManager manager;

    // Create a space to associate the version with
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    spaceManager.addSpace(space);

    // Create a version using the DTO manager
    std::string inputJson = R"({
        "name": "Version 1.0",
        "description": "Initial version",
        "tag": "v1.0"
    })";

    manager.createVersion("Test Space", inputJson);

    // Verify that the version was created successfully
    auto spaces = spaceManager.getAllSpaces();
    ASSERT_FALSE(spaces.empty());

    int spaceId = spaces[0].id;
    auto versions = VersionManager::getInstance().getVersionsBySpaceId(spaceId, 0, std::numeric_limits<int>::max());
    ASSERT_EQ(versions.size(), 1);
    EXPECT_EQ(versions[0].name, "Version 1.0");
    EXPECT_EQ(versions[0].description, "Initial version");
    EXPECT_EQ(versions[0].tag, "v1.0");
    EXPECT_TRUE(versions[0].is_default);
    EXPECT_EQ(versions[0].unique_id, 1);
}

// Test for getting a version by its unique_id and space name
TEST_F(VersionServiceManagerTest, GetByVersionId) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionServiceManager manager;

    // Create a space and associate a version with it
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    spaceManager.addSpace(space);

    std::string inputJson = R"({
        "name": "Version 1.0",
        "description": "Initial version",
        "tag": "v1.0"
    })";

    manager.createVersion("Test Space", inputJson);

    // Retrieve the version by spaceName and unique_id
    auto versionJson = manager.getByVersionId("Test Space", 1);

    // Validate the JSON output
    ASSERT_TRUE(versionJson.contains("id"));
    ASSERT_EQ(versionJson["name"], "Version 1.0");
    ASSERT_EQ(versionJson["description"], "Initial version");
    ASSERT_EQ(versionJson["tag"], "v1.0");
    ASSERT_TRUE(versionJson["is_default"]);
}

// Test for getting a version by its name and space name
TEST_F(VersionServiceManagerTest, GetByVersionName) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionServiceManager manager;

    // Create a space and associate a version with it
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    spaceManager.addSpace(space);

    std::string inputJson = R"({
        "name": "Version 1.0",
        "description": "Initial version",
        "tag": "v1.0"
    })";

    manager.createVersion("Test Space", inputJson);

    // Retrieve the version by name and space name
    auto versionJson = manager.getByVersionName("Test Space", "Version 1.0");

    // Validate the JSON output
    ASSERT_TRUE(versionJson.contains("id"));
    ASSERT_EQ(versionJson["name"], "Version 1.0");
    ASSERT_EQ(versionJson["description"], "Initial version");
    ASSERT_EQ(versionJson["tag"], "v1.0");
    ASSERT_TRUE(versionJson["is_default"]);
}

// Test for getting the default version for a space
TEST_F(VersionServiceManagerTest, GetDefaultVersion) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionServiceManager manager;

    // Create a space and associate a default version with it
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    spaceManager.addSpace(space);

    std::string inputJson = R"({
        "name": "Default Version",
        "description": "This is the default version",
        "tag": "default"
    })";

    manager.createVersion("Test Space", inputJson);

    // Retrieve the default version by space name
    auto defaultVersionJson = manager.getDefaultVersion("Test Space");

    // Validate the JSON output
    ASSERT_TRUE(defaultVersionJson.contains("id"));
    ASSERT_EQ(defaultVersionJson["name"], "Default Version");
    ASSERT_EQ(defaultVersionJson["description"], "This is the default version");
    ASSERT_EQ(defaultVersionJson["tag"], "default");
    ASSERT_TRUE(defaultVersionJson["is_default"]);
}

// Test for getting a list of versions by space name
TEST_F(VersionServiceManagerTest, GetLists) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionServiceManager manager;

    // Create a space and associate multiple versions with it
    Space space(0, "VersionServiceManagerTest_GetLists", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    spaceManager.addSpace(space);

    std::string inputJson1 = R"({
        "name": "Version 1.0",
        "description": "Initial version",
        "tag": "v1.0"
    })";

    std::string inputJson2 = R"({
        "name": "Version 2.0",
        "description": "Second version",
        "tag": "v2.0"
    })";

    manager.createVersion("VersionServiceManagerTest_GetLists", inputJson1);
    manager.createVersion("VersionServiceManagerTest_GetLists", inputJson2);

    // Retrieve the list of versions by space name
    auto versionListJson = manager.getLists("VersionServiceManagerTest_GetLists", 0, std::numeric_limits<int>::max());

    // Validate the JSON output
    ASSERT_TRUE(versionListJson.contains("values"));
    auto valuesArray = versionListJson["values"];
    ASSERT_EQ(valuesArray.size(), 2);

    bool version1Found = false;
    bool version2Found = false;

    for (const auto& versionEntry : valuesArray) {
        if (versionEntry["name"] == "Version 1.0") {
            version1Found = true;
        }
        if (versionEntry["name"] == "Version 2.0") {
            version2Found = true;
        }
    }

    EXPECT_TRUE(version1Found);
    EXPECT_TRUE(version2Found);
}

// Test for handling a non-existent version by unique_id and space name
TEST_F(VersionServiceManagerTest, GetByVersionId_NonExistent) {
    VersionServiceManager manager;

    // Attempt to retrieve a non-existent version
    EXPECT_THROW(manager.getByVersionId("Test Space", 999), std::runtime_error);
}

// Test for handling a non-existent space
TEST_F(VersionServiceManagerTest, GetByVersionId_NonExistentSpace) {
    VersionServiceManager manager;

    // Attempt to retrieve a version from a non-existent space
    EXPECT_THROW(manager.getByVersionId("NonExistent Space", 1), std::runtime_error);
}

// Additional Test: Creating multiple versions and verifying unique_id increment
TEST_F(VersionServiceManagerTest, UniqueIdIncrement) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionServiceManager manager;

    // Create a space
    Space space(0, "VersionServiceManagerTest_UniqueIdIncrement", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    spaceManager.addSpace(space);

    // Create multiple versions
    std::string inputJson1 = R"({
        "name": "Version 1.0",
        "description": "First version",
        "tag": "v1.0"
    })";

    std::string inputJson2 = R"({
        "name": "Version 2.0",
        "description": "Second version",
        "tag": "v2.0"
    })";

    std::string inputJson3 = R"({
        "name": "Version 3.0",
        "description": "Third version",
        "tag": "v3.0"
    })";

    manager.createVersion("VersionServiceManagerTest_UniqueIdIncrement", inputJson1);
    manager.createVersion("VersionServiceManagerTest_UniqueIdIncrement", inputJson2);
    manager.createVersion("VersionServiceManagerTest_UniqueIdIncrement", inputJson3);

    // Retrieve all versions
    auto versions = VersionManager::getInstance().getVersionsBySpaceId(space.id, 0, std::numeric_limits<int>::max());
    ASSERT_EQ(versions.size(), 3);

    // Verify unique_id increments correctly
    EXPECT_EQ(versions[0].unique_id, 3);
    EXPECT_EQ(versions[1].unique_id, 2);
    EXPECT_EQ(versions[2].unique_id, 1);
}

// Test for deleting a version by its unique_id and space name
TEST_F(VersionServiceManagerTest, DeleteByVersionId) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionServiceManager manager;

    // Create a space and associate a version with it
    Space space(0, "Test Space", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
    spaceManager.addSpace(space);

    std::string inputJson = R"({
        "name": "Version 1.0",
        "description": "Initial version",
        "tag": "v1.0"
    })";

    manager.createVersion("Test Space", inputJson);

    // Verify the version was created
    auto versionJson = manager.getByVersionId("Test Space", 1);
    ASSERT_TRUE(versionJson.contains("id"));

    // Delete the version
    manager.deleteByVersionId("Test Space", 1);

    // Verify the version was deleted
    EXPECT_THROW(manager.getByVersionId("Test Space", 1), std::runtime_error);
}