#include <gtest/gtest.h>
#include "service/SnapshotService.hpp"
#include "service/SearchService.hpp"
#include "service/SpaceService.hpp"
#include "service/VectorService.hpp"
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

// Fixture for SnapshotServiceManager tests
class SnapshotServiceManagerTest : public ::testing::Test {
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

        // Clear snapshot directory
        std::string snapshotDirectory = Config::getInstance().getDataPath() + "/snapshot/";
        std::filesystem::remove_all(snapshotDirectory);
    }

    void TearDown() override {
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM RbacToken;");

        // Clear snapshot directory
        std::string snapshotDirectory = Config::getInstance().getDataPath() + "/snapshot/";
        std::filesystem::remove_all(snapshotDirectory);
    }
};

// Test for creating a snapshot with a given space name and version unique ID
TEST_F(SnapshotServiceManagerTest, CreateSnapshot) {
    // Create a space
    std::string inputJson = R"({
        "name": "CreateSnapshot",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceServiceManager spaceServiceManager;
    spaceServiceManager.createSpace(inputJson);

    // Insert some vectors for search
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"}
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"}
            }
        ]
    })";

    VectorServiceManager vectorServiceManager;
    vectorServiceManager.upsert("CreateSnapshot", 1, vectorDataJson);

    // Create a snapshot using the DTO manager
    SnapshotServiceManager manager;
    std::string snapshotInputJson = R"({
        "CreateSnapshot": 1
    })";
    manager.createSnapshot(snapshotInputJson);

    // Verify that the snapshot file was created
    auto snapshotList = manager.listSnapshots();
    ASSERT_TRUE(snapshotList.contains("snapshots"));
    ASSERT_GT(snapshotList["snapshots"].size(), 0);
}

// Test for restoring a snapshot using file name
TEST_F(SnapshotServiceManagerTest, RestoreSnapshot) {
    // Create a space
    std::string inputJson = R"({
        "name": "RestoreSnapshot",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceServiceManager spaceServiceManager;
    spaceServiceManager.createSpace(inputJson);

    // Insert some vectors for search
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"}
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"}
            }
        ]
    })";

    VectorServiceManager vectorServiceManager;
    vectorServiceManager.upsert("RestoreSnapshot", 1, vectorDataJson);

    // Create a snapshot
    SnapshotServiceManager manager;
    std::string snapshotInputJson = R"({
        "RestoreSnapshot": 1
    })";
    manager.createSnapshot(snapshotInputJson);

    // Get the created snapshot file name
    auto snapshotList = manager.listSnapshots();
    ASSERT_TRUE(snapshotList.contains("snapshots"));
    ASSERT_GT(snapshotList["snapshots"].size(), 0);

    // Print out the snapshot details for debugging
    spdlog::info("Snapshot list: {}", snapshotList.dump());

    // Extract the correct zip file name from the snapshot list
    std::string fileName = snapshotList["snapshots"][0]["file_name"];
    ASSERT_FALSE(fileName.empty());
    spdlog::info("Using snapshot file name: {}", fileName);

    // Delete all data from Space and Version tables
    auto& db = DatabaseManager::getInstance().getDatabase();
    db.exec("DELETE FROM Space;");
    db.exec("DELETE FROM Version;");

    // Restore the snapshot using the file name
    manager.restoreSnapshot(fileName);

    // Verify that the data is restored correctly
    json spaceJson = spaceServiceManager.getBySpaceName("RestoreSnapshot");

    // Validate the JSON output
    ASSERT_EQ(spaceJson["name"], "RestoreSnapshot");

    // Query to vector space
    std::string queryJsonStr = R"({
        "vector": [0.25, 0.45, 0.75, 0.85]
    })";

    SearchServiceManager searchServiceManager;
    auto searchResults = searchServiceManager.search("RestoreSnapshot", 0, queryJsonStr, 2); // default version

    // Validate that two results are returned
    ASSERT_EQ(searchResults.size(), 2);
}

// Test for listing snapshots
TEST_F(SnapshotServiceManagerTest, ListSnapshots) {
    // Create a space
    std::string inputJson = R"({
        "name": "ListSnapshots",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceServiceManager spaceServiceManager;
    spaceServiceManager.createSpace(inputJson);

    // Insert some vectors for search
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"}
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"}
            }
        ]
    })";

    VectorServiceManager vectorServiceManager;
    vectorServiceManager.upsert("ListSnapshots", 1, vectorDataJson);

    // Create a snapshot
    SnapshotServiceManager manager;
    std::string snapshotInputJson = R"({
        "ListSnapshots": 1
    })";
    manager.createSnapshot(snapshotInputJson);

    // List snapshots and verify output
    auto snapshotList = manager.listSnapshots();
    ASSERT_TRUE(snapshotList.contains("snapshots"));
    ASSERT_GT(snapshotList["snapshots"].size(), 0);
}

// Test for deleting snapshots
TEST_F(SnapshotServiceManagerTest, DeleteSnapshots) {
    // Create a space
    std::string inputJson = R"({
        "name": "DeleteSnapshots",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceServiceManager spaceServiceManager;
    spaceServiceManager.createSpace(inputJson);

    // Insert some vectors for search
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"}
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"}
            }
        ]
    })";

    VectorServiceManager vectorServiceManager;
    vectorServiceManager.upsert("DeleteSnapshots", 1, vectorDataJson);

    // Create a snapshot
    SnapshotServiceManager manager;
    std::string snapshotInputJson = R"({
        "DeleteSnapshots": 1
    })";
    manager.createSnapshot(snapshotInputJson);

    // Verify the snapshot is listed
    auto snapshotList = manager.listSnapshots();
    ASSERT_GT(snapshotList["snapshots"].size(), 0);

    // Delete snapshots
    manager.deleteSnapshots();

    // Verify snapshots are deleted
    snapshotList = manager.listSnapshots();
    ASSERT_EQ(snapshotList["snapshots"].size(), 0);
}

TEST_F(SnapshotServiceManagerTest, DeleteSnapshotByFileName) {
    // Create a space
    std::string inputJson = R"({
        "name": "DeleteSnapshotTest",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceServiceManager spaceServiceManager;
    spaceServiceManager.createSpace(inputJson);

    // Insert some vectors for search
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"}
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"}
            }
        ]
    })";

    VectorServiceManager vectorServiceManager;
    vectorServiceManager.upsert("DeleteSnapshotTest", 1, vectorDataJson);

    // Create a snapshot
    SnapshotServiceManager manager;
    std::string snapshotInputJson = R"({
        "DeleteSnapshotTest": 1
    })";
    manager.createSnapshot(snapshotInputJson);

    // Get the created snapshot file name
    auto snapshotList = manager.listSnapshots();
    ASSERT_TRUE(snapshotList.contains("snapshots"));
    ASSERT_GT(snapshotList["snapshots"].size(), 0);

    // Extract the file name
    std::string fileName = snapshotList["snapshots"][0]["file_name"];
    ASSERT_FALSE(fileName.empty());

    // Delete the specific snapshot by filename
    manager.deleteSnapshot(fileName);

    // List snapshots again and verify that the file is deleted
    snapshotList = manager.listSnapshots();
    bool fileFound = false;
    for (const auto& snapshot : snapshotList["snapshots"]) {
        if (snapshot["file_name"] == fileName) {
            fileFound = true;
            break;
        }
    }
    ASSERT_FALSE(fileFound) << "The snapshot file " << fileName << " was not deleted as expected.";
}