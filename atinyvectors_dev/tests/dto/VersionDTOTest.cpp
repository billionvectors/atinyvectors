#include <gtest/gtest.h>
#include "Version.hpp"
#include "Space.hpp"
#include "DatabaseManager.hpp"
#include "dto/VersionDTO.hpp"
#include "nlohmann/json.hpp"

using namespace atinyvectors;
using namespace atinyvectors::dto;
using namespace nlohmann;

long getCurrentTimeUTC() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// Fixture for VersionDTOManager tests
class VersionDTOManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        VersionManager& versionManager = VersionManager::getInstance();
        SpaceManager& spaceManager = SpaceManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM Space;");
    }

    void TearDown() override {
        VersionManager& versionManager = VersionManager::getInstance();
        SpaceManager& spaceManager = SpaceManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM Space;");
    }
};

// Test for creating a version with a given space name
TEST_F(VersionDTOManagerTest, CreateVersion) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionDTOManager manager;

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
    auto versions = VersionManager::getInstance().getVersionsBySpaceId(spaceId);
    ASSERT_EQ(versions.size(), 1);
    EXPECT_EQ(versions[0].name, "Version 1.0");
    EXPECT_EQ(versions[0].description, "Initial version");
    EXPECT_EQ(versions[0].tag, "v1.0");
    EXPECT_TRUE(versions[0].is_default);
    EXPECT_EQ(versions[0].unique_id, 1);
}

// Test for getting a version by its unique_id and space name
TEST_F(VersionDTOManagerTest, GetByVersionId) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionDTOManager manager;

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
    ASSERT_TRUE(versionJson.contains("versionId"));
    ASSERT_EQ(versionJson["name"], "Version 1.0");
    ASSERT_EQ(versionJson["description"], "Initial version");
    ASSERT_EQ(versionJson["tag"], "v1.0");
    ASSERT_TRUE(versionJson["is_default"]);
}

// Test for getting a version by its name and space name
TEST_F(VersionDTOManagerTest, GetByVersionName) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionDTOManager manager;

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
    ASSERT_TRUE(versionJson.contains("versionId"));
    ASSERT_EQ(versionJson["name"], "Version 1.0");
    ASSERT_EQ(versionJson["description"], "Initial version");
    ASSERT_EQ(versionJson["tag"], "v1.0");
    ASSERT_TRUE(versionJson["is_default"]);
}

// Test for getting the default version for a space
TEST_F(VersionDTOManagerTest, GetDefaultVersion) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionDTOManager manager;

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
    ASSERT_TRUE(defaultVersionJson.contains("versionId"));
    ASSERT_EQ(defaultVersionJson["name"], "Default Version");
    ASSERT_EQ(defaultVersionJson["description"], "This is the default version");
    ASSERT_EQ(defaultVersionJson["tag"], "default");
    ASSERT_TRUE(defaultVersionJson["is_default"]);
}

// Test for getting a list of versions by space name
TEST_F(VersionDTOManagerTest, GetLists) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionDTOManager manager;

    // Create a space and associate multiple versions with it
    Space space(0, "VersionDTOManagerTest_GetLists", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
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

    manager.createVersion("VersionDTOManagerTest_GetLists", inputJson1);
    manager.createVersion("VersionDTOManagerTest_GetLists", inputJson2);

    // Retrieve the list of versions by space name
    auto versionListJson = manager.getLists("VersionDTOManagerTest_GetLists");

    // Validate the JSON output
    ASSERT_TRUE(versionListJson.contains("values"));
    auto valuesArray = versionListJson["values"];
    ASSERT_EQ(valuesArray.size(), 2);

    bool version1Found = false;
    bool version2Found = false;

    for (const auto& versionEntry : valuesArray) {
        if (versionEntry.contains("Version 1.0")) {
            version1Found = true;
        }
        if (versionEntry.contains("Version 2.0")) {
            version2Found = true;
        }
    }

    EXPECT_TRUE(version1Found);
    EXPECT_TRUE(version2Found);
}

// Test for handling a non-existent version by unique_id and space name
TEST_F(VersionDTOManagerTest, GetByVersionId_NonExistent) {
    VersionDTOManager manager;

    // Attempt to retrieve a non-existent version
    EXPECT_THROW(manager.getByVersionId("Test Space", 999), std::runtime_error);
}

// Test for handling a non-existent space
TEST_F(VersionDTOManagerTest, GetByVersionId_NonExistentSpace) {
    VersionDTOManager manager;

    // Attempt to retrieve a version from a non-existent space
    EXPECT_THROW(manager.getByVersionId("NonExistent Space", 1), std::runtime_error);
}

// Additional Test: Creating multiple versions and verifying unique_id increment
TEST_F(VersionDTOManagerTest, UniqueIdIncrement) {
    SpaceManager& spaceManager = SpaceManager::getInstance();
    VersionDTOManager manager;

    // Create a space
    Space space(0, "VersionDTOManagerTest_UniqueIdIncrement", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
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

    manager.createVersion("VersionDTOManagerTest_UniqueIdIncrement", inputJson1);
    manager.createVersion("VersionDTOManagerTest_UniqueIdIncrement", inputJson2);
    manager.createVersion("VersionDTOManagerTest_UniqueIdIncrement", inputJson3);

    // Retrieve all versions
    auto versions = VersionManager::getInstance().getVersionsBySpaceId(space.id);
    ASSERT_EQ(versions.size(), 3);

    // Verify unique_id increments correctly
    EXPECT_EQ(versions[0].unique_id, 1);
    EXPECT_EQ(versions[1].unique_id, 2);
    EXPECT_EQ(versions[2].unique_id, 3);
}