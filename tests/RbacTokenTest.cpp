#include <gtest/gtest.h>
#include "algo/HnswIndexLRUCache.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Snapshot.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "RbacToken.hpp"

using namespace atinyvectors;
using namespace atinyvectors::algo;

// Fixture for RbacTokenManager tests
class RbacTokenManagerTest : public ::testing::Test {
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
        HnswIndexLRUCache::getInstance().clean();

        expireDays = 0;
    }

    void TearDown() override {
        // Clear data after each test
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM RbacToken;");
    }

    int expireDays;
};

// Test for creating a new token and verifying its existence
TEST_F(RbacTokenManagerTest, CreateNewToken) {
    RbacTokenManager& tokenManager = RbacTokenManager::getInstance();

    // Create a new token
    RbacToken token = tokenManager.newToken(1, Permission::ReadWrite, Permission::ReadOnly, 
                                            Permission::ReadOnly, Permission::ReadOnly, Permission::ReadWrite, 
                                            expireDays);
    EXPECT_GT(token.id, 0);
    EXPECT_EQ(token.user_id, 1);
    EXPECT_EQ(token.system_permission, Permission::ReadWrite);
    EXPECT_EQ(token.space_permission, Permission::ReadOnly);

    // Retrieve the token by token string
    RbacToken retrieved = tokenManager.getTokenByToken(token.token);
    EXPECT_EQ(retrieved.id, token.id);
    EXPECT_EQ(retrieved.token, token.token);
    EXPECT_EQ(retrieved.user_id, 1);
    EXPECT_EQ(retrieved.system_permission, Permission::ReadWrite);
    EXPECT_EQ(retrieved.space_permission, Permission::ReadOnly);
}

// Test for deleting a token by token string
TEST_F(RbacTokenManagerTest, DeleteTokenByToken) {
    RbacTokenManager& tokenManager = RbacTokenManager::getInstance();

    // Create a new token
    RbacToken token = tokenManager.newToken(2, Permission::ReadOnly, Permission::ReadWrite,
                                            Permission::ReadOnly, Permission::ReadOnly, Permission::ReadOnly,
                                            expireDays);
    EXPECT_GT(token.id, 0);

    // Ensure the token is added
    RbacToken retrieved = tokenManager.getTokenByToken(token.token);
    EXPECT_EQ(retrieved.id, token.id);

    // Delete the token by token string
    tokenManager.deleteByToken(token.token);

    // Ensure the token is deleted
    EXPECT_THROW(tokenManager.getTokenByToken(token.token), std::runtime_error);
}

// Test for deleting all expired tokens
TEST_F(RbacTokenManagerTest, DeleteAllExpiredTokens) {
    RbacTokenManager& tokenManager = RbacTokenManager::getInstance();

    // Create an expired token
    RbacToken expiredToken = tokenManager.newToken(3, Permission::ReadOnly, Permission::ReadOnly, 
                                                   Permission::ReadOnly, Permission::ReadOnly, Permission::ReadOnly,
                                                   1); // 1 day expiry
    expiredToken.expire_time_utc = std::time(nullptr) - 10; // Manually set to expired 10 seconds ago
    tokenManager.updateToken(expiredToken); // Update the expiration time

    // Create a non-expired token
    RbacToken validToken = tokenManager.newToken(4, Permission::ReadWrite, Permission::ReadWrite,
                                                 Permission::ReadWrite, Permission::ReadWrite, Permission::ReadWrite,
                                                 10); // 10 days expiry

    // Delete all expired tokens
    tokenManager.deleteAllExpireTokens();

    // Check the expired token is deleted
    EXPECT_THROW(tokenManager.getTokenByToken(expiredToken.token), std::runtime_error);

    // Check the non-expired token still exists
    RbacToken retrieved = tokenManager.getTokenByToken(validToken.token);
    EXPECT_EQ(retrieved.user_id, 4);
}
