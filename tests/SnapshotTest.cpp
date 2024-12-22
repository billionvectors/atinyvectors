#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <zip.h>

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

using namespace atinyvectors;
using namespace atinyvectors::algo;
using namespace atinyvectors::utils;
namespace fs = std::filesystem;

class SnapshotManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        IdCache::getInstance().clean();

        // clean data
        DatabaseManager::getInstance().reset();
        FaissIndexLRUCache::getInstance().clean();

        // Create a temporary metaDirectory for testing
        metaDirectory = "test_meta_directory";
        fs::create_directories(metaDirectory);

        // Create a test file in the metaDirectory
        testFilePath = metaDirectory + "/test_file.txt";
        std::ofstream testFile(testFilePath);
        testFile << "This is a test file for the snapshot." << std::endl;
        testFile.close();

        // create dummy data
        // Add a new space and capture the returned spaceId
        Space space(0, "CreateAndRestoreSnapshot", "A space for testing", getCurrentTimeUTC(), getCurrentTimeUTC());
        spaceId = SpaceManager::getInstance().addSpace(space);

        // Add a version to the space
        Version version;
        version.spaceId = spaceId;
        version.name = "Version 1.0";
        version.description = "Initial version";
        version.tag = "v1.0";
        version.is_default = true;

        versionId = VersionManager::getInstance().addVersion(version);

        HnswConfig hnswConfig(16, 200);
        QuantizationConfig quantizationConfig;

        // Add a new vector index
        VectorIndex vectorIndex(0, versionId, VectorValueType::Dense, "Vector Index 1", MetricType::L2, 128,
                                hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
        vectorIndexId = VectorIndexManager::getInstance().addVectorIndex(vectorIndex);

        IdCache& cache = IdCache::getInstance();
        versionUniqueId = cache.getDefaultUniqueVersionId("CreateAndRestoreSnapshot");
    }

    void TearDown() override {
        // Clean up the metaDirectory after tests
        fs::remove_all(metaDirectory);
    }

    // Helper function to check if manifest.json exists in a zip file and validate its content
    bool checkManifestInZip(const std::string& zipFileName) {
        int err = 0;
        zip_t* zip = zip_open(zipFileName.c_str(), ZIP_RDONLY, &err);
        if (!zip) {
            spdlog::error("Failed to open ZIP file: {}", zipFileName);
            return false;
        }

        // Check if manifest.json exists
        zip_int64_t index = zip_name_locate(zip, "manifest.json", ZIP_FL_NODIR);
        if (index < 0) {
            zip_close(zip);
            spdlog::error("manifest.json not found in the ZIP file: {}", zipFileName);
            return false;
        }

        // Open manifest.json file in the zip
        zip_file_t* manifestFile = zip_fopen_index(zip, index, 0);
        if (!manifestFile) {
            zip_close(zip);
            spdlog::error("Failed to open manifest.json in ZIP file: {}", zipFileName);
            return false;
        }

        // Read the manifest.json content
        std::stringstream manifestContent;
        char buffer[1024];
        zip_int64_t bytesRead;
        while ((bytesRead = zip_fread(manifestFile, buffer, sizeof(buffer))) > 0) {
            manifestContent.write(buffer, bytesRead);
        }

        zip_fclose(manifestFile);
        zip_close(zip);

        // Parse and validate JSON content
        try {
            nlohmann::json manifestJson = nlohmann::json::parse(manifestContent.str());
            EXPECT_TRUE(manifestJson.contains("version"));
            EXPECT_TRUE(manifestJson.contains("create_date"));
            EXPECT_TRUE(manifestJson.contains("indexes"));
            EXPECT_GT(manifestJson["indexes"].size(), 0);

            for (const auto& index : manifestJson["indexes"]) {
                EXPECT_TRUE(index.contains("spaceName"));
                EXPECT_TRUE(index.contains("versionId"));
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to parse or validate manifest.json: {}", e.what());
            return false;
        }

        return true;
    }

    int spaceId;
    int versionId;
    int versionUniqueId;
    int vectorIndexId;

    std::string metaDirectory;
    std::string testFilePath;
};

// Test for creating and restoring a snapshot
TEST_F(SnapshotManagerTest, CreateAndRestoreSnapshot) {
    SnapshotManager& snapshotManager = SnapshotManager::getInstance();
    VersionManager& versionManager = VersionManager::getInstance();
    SpaceManager& spaceManager = SpaceManager::getInstance();

    // Create a snapshot for the version, including metaDirectory
    std::vector<std::pair<std::string, int>> versionInfoList = {{"CreateAndRestoreSnapshot", versionUniqueId}};
    std::string snapshotFileName = "CreateAndRestoreSnapshot.zip";
    int snapshotId = snapshotManager.createSnapshot(versionInfoList, snapshotFileName, metaDirectory);
    EXPECT_GT(snapshotId, 0);

    // Verify snapshot creation
    Snapshot snapshot = snapshotManager.getSnapshotById(snapshotId);
    EXPECT_FALSE(snapshot.fileName.empty());
    EXPECT_FALSE(snapshot.requestJson.empty());

    // Verify manifest.json is created in the ZIP
    ASSERT_TRUE(checkManifestInZip(snapshot.fileName));

    // Delete all data from Space and Version tables
    auto& db = DatabaseManager::getInstance().getDatabase();
    db.exec("DELETE FROM Space;");
    db.exec("DELETE FROM Version;");

    // Delete the test file to simulate a lost file scenario
    fs::remove(testFilePath);

    // Verify the data is deleted
    EXPECT_THROW(spaceManager.getSpaceById(spaceId), std::runtime_error);
    EXPECT_THROW(versionManager.getVersionByUniqueId(spaceId, versionUniqueId), std::runtime_error);

    // Verify that the test file is deleted
    EXPECT_FALSE(fs::exists(testFilePath));

    // Restore the snapshot
    snapshotManager.restoreSnapshot(snapshot.fileName, metaDirectory);

    // Verify that the data is restored correctly
    Space restoredSpace = spaceManager.getSpaceById(spaceId);
    EXPECT_EQ(restoredSpace.name, "CreateAndRestoreSnapshot");
    EXPECT_EQ(restoredSpace.description, "A space for testing");

    Version restoredVersion = versionManager.getVersionByUniqueId(spaceId, versionUniqueId);
    EXPECT_EQ(restoredVersion.name, "Version 1.0");
    EXPECT_EQ(restoredVersion.description, "Initial version");
    EXPECT_TRUE(restoredVersion.is_default);

    // Verify that the test file in the metaDirectory is restored correctly
    std::ifstream restoredTestFile(testFilePath);
    ASSERT_TRUE(restoredTestFile.good());
    
    std::string restoredContent;
    std::getline(restoredTestFile, restoredContent);
    EXPECT_EQ(restoredContent, "This is a test file for the snapshot.");
    restoredTestFile.close();
}