#include <gtest/gtest.h>
#include "IdCache.hpp"
#include "DatabaseManager.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Snapshot.hpp"

using namespace atinyvectors;

class IdCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        DatabaseManager::getInstance().reset();

        createDummyData();
    }

    void TearDown() override {
        IdCache::getInstance().clean();
    }

    void createDummyData() {
        // Use SpaceManager and VersionManager to create test data
        SpaceManager& spaceManager = SpaceManager::getInstance();
        VersionManager& versionManager = VersionManager::getInstance();

        // Create a new space
        Space testSpace(0, "exampleSpace", "A test space", 1628000000, 1628000000);
        spaceId = spaceManager.addSpace(testSpace);  // Use returned spaceId

        versionUniqueId = 1;

        // Create a version associated with the space
        Version version;
        version.spaceId = spaceId;  // Correctly use the returned spaceId
        version.unique_id = versionUniqueId;
        version.name = "v1.0";
        version.description = "First version";
        version.tag = "alpha";
        version.created_time_utc = 1628000000;
        version.updated_time_utc = 1628000000;
        version.is_default = true;  // Mark this version as the default
        versionId = versionManager.addVersion(version);  // Use returned versionId
    }

    int spaceId;
    int versionId;
    int versionUniqueId;
};

TEST_F(IdCacheTest, TestGet) {
    IdCache& cache = IdCache::getInstance();
    
    // Test fetching data using spaceName and versionUniqueId
    int fetchedVersionId = cache.getVersionId("exampleSpace", versionUniqueId);
    EXPECT_EQ(fetchedVersionId, versionId); // The expected versionId should match the one returned by addVersion
}

TEST_F(IdCacheTest, TestGetDefaultVersion) {
    IdCache& cache = IdCache::getInstance();
    
    int fetchedVersionId = cache.getDefaultVersionId("exampleSpace");
    EXPECT_EQ(fetchedVersionId, versionId); // The expected versionId should be the default version ID
}

TEST_F(IdCacheTest, TestGetByVersionId) {
    IdCache& cache = IdCache::getInstance();

    // Test fetching data using versionId
    auto result = cache.getSpaceNameAndVersionUniqueId(versionId);
    EXPECT_EQ(result.first, "exampleSpace");
    EXPECT_EQ(result.second, versionUniqueId);
}

TEST_F(IdCacheTest, TestCacheMissAndDBFetch) {
    IdCache& cache = IdCache::getInstance();

    // Simulate a cache miss by using a non-cached item
    int fetchedVersionId = cache.getVersionId("exampleSpace", versionUniqueId); // Should load from DB and cache it

    // Check if cache is correctly updated after DB fetch
    int cachedVersionId = cache.getVersionId("exampleSpace", versionUniqueId);
    EXPECT_EQ(cachedVersionId, fetchedVersionId);
}

TEST_F(IdCacheTest, TestCleanCache) {
    IdCache& cache = IdCache::getInstance();

    // Fill the cache
    cache.getVersionId("exampleSpace", versionUniqueId);
    
    // Clean the cache
    cache.clean();

    // The cache should be empty now, and next access should result in DB fetch again.
    EXPECT_THROW(cache.getSpaceNameAndVersionUniqueId(9999), std::runtime_error); // This should throw since it's not in DB
}

TEST_F(IdCacheTest, TestNonExistentEntry) {
    IdCache& cache = IdCache::getInstance();

    // Test accessing a non-existent entry
    EXPECT_THROW(cache.getVersionId("nonExistentSpace", 999), std::runtime_error);
}
