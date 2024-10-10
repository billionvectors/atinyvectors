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

        // Clean data
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

// Helper function to create permissions
struct Permissions {
    Permission system_permission;
    Permission space_permission;
    Permission version_permission;
    Permission vector_permission;
    Permission search_permission;
    Permission snapshot_permission;
    Permission security_permission;
    Permission keyvalue_permission;
};

// Test for creating a new token and verifying its existence
TEST_F(RbacTokenManagerTest, CreateNewToken) {
    RbacTokenManager& tokenManager = RbacTokenManager::getInstance();

    // Define permissions
    Permissions perms = {
        Permission::ReadWrite,   // system_permission
        Permission::ReadOnly,    // space_permission
        Permission::ReadOnly,    // version_permission
        Permission::ReadOnly,    // vector_permission
        Permission::ReadWrite,   // search_permission
        Permission::ReadOnly,    // snapshot_permission
        Permission::ReadWrite,   // security_permission
        Permission::ReadOnly     // keyvalue_permission
    };

    // Create a new token with all permissions
    RbacToken token = tokenManager.newToken(
        1,                              // space_id
        perms.system_permission,        // system_permission
        perms.space_permission,         // space_permission
        perms.version_permission,       // version_permission
        perms.vector_permission,        // vector_permission
        perms.search_permission,        // search_permission
        perms.snapshot_permission,      // snapshot_permission
        perms.security_permission,      // security_permission
        perms.keyvalue_permission,      // keyvalue_permission
        expireDays                       // expireDays
    );

    // Verify that the token was created successfully
    EXPECT_GT(token.id, 0);
    EXPECT_EQ(token.space_id, 1);
    EXPECT_EQ(token.system_permission, perms.system_permission);
    EXPECT_EQ(token.space_permission, perms.space_permission);
    EXPECT_EQ(token.version_permission, perms.version_permission);
    EXPECT_EQ(token.vector_permission, perms.vector_permission);
    EXPECT_EQ(token.search_permission, perms.search_permission);
    EXPECT_EQ(token.snapshot_permission, perms.snapshot_permission);
    EXPECT_EQ(token.security_permission, perms.security_permission);
    EXPECT_EQ(token.keyvalue_permission, perms.keyvalue_permission);

    // Retrieve the token by token string
    RbacToken retrieved = tokenManager.getTokenByToken(token.token);
    EXPECT_EQ(retrieved.id, token.id);
    EXPECT_EQ(retrieved.token, token.token);
    EXPECT_EQ(retrieved.space_id, 1);
    EXPECT_EQ(retrieved.system_permission, perms.system_permission);
    EXPECT_EQ(retrieved.space_permission, perms.space_permission);
    EXPECT_EQ(retrieved.version_permission, perms.version_permission);
    EXPECT_EQ(retrieved.vector_permission, perms.vector_permission);
    EXPECT_EQ(retrieved.search_permission, perms.search_permission);
    EXPECT_EQ(retrieved.snapshot_permission, perms.snapshot_permission);
    EXPECT_EQ(retrieved.security_permission, perms.security_permission);
    EXPECT_EQ(retrieved.keyvalue_permission, perms.keyvalue_permission);
}

// Test for deleting a token by token string
TEST_F(RbacTokenManagerTest, DeleteTokenByToken) {
    RbacTokenManager& tokenManager = RbacTokenManager::getInstance();

    // Define permissions
    Permissions perms = {
        Permission::ReadOnly,    // system_permission
        Permission::ReadWrite,   // space_permission
        Permission::ReadOnly,    // version_permission
        Permission::ReadOnly,    // vector_permission
        Permission::ReadOnly,    // search_permission
        Permission::ReadWrite,   // snapshot_permission
        Permission::ReadOnly,    // security_permission
        Permission::ReadWrite    // keyvalue_permission
    };

    // Create a new token with all permissions
    RbacToken token = tokenManager.newToken(
        2,                              // user_id
        perms.system_permission,        // system_permission
        perms.space_permission,         // space_permission
        perms.version_permission,       // version_permission
        perms.vector_permission,        // vector_permission
        perms.search_permission,        // search_permission
        perms.snapshot_permission,      // snapshot_permission
        perms.security_permission,      // security_permission
        perms.keyvalue_permission,      // keyvalue_permission
        expireDays                       // expireDays
    );

    // Verify that the token was created successfully
    EXPECT_GT(token.id, 0);

    // Ensure the token is added
    RbacToken retrieved = tokenManager.getTokenByToken(token.token);
    EXPECT_EQ(retrieved.id, token.id);

    // Verify all permissions
    EXPECT_EQ(retrieved.system_permission, perms.system_permission);
    EXPECT_EQ(retrieved.space_permission, perms.space_permission);
    EXPECT_EQ(retrieved.version_permission, perms.version_permission);
    EXPECT_EQ(retrieved.vector_permission, perms.vector_permission);
    EXPECT_EQ(retrieved.search_permission, perms.search_permission);
    EXPECT_EQ(retrieved.snapshot_permission, perms.snapshot_permission);
    EXPECT_EQ(retrieved.security_permission, perms.security_permission);
    EXPECT_EQ(retrieved.keyvalue_permission, perms.keyvalue_permission);

    // Delete the token by token string
    tokenManager.deleteByToken(token.token);

    // Ensure the token is deleted
    EXPECT_THROW(tokenManager.getTokenByToken(token.token), std::runtime_error);
}

// Test for deleting all expired tokens
TEST_F(RbacTokenManagerTest, DeleteAllExpiredTokens) {
    RbacTokenManager& tokenManager = RbacTokenManager::getInstance();

    // Define permissions for expired token
    Permissions expiredPerms = {
        Permission::ReadOnly,    // system_permission
        Permission::ReadOnly,    // space_permission
        Permission::ReadOnly,    // version_permission
        Permission::ReadOnly,    // vector_permission
        Permission::ReadOnly,    // search_permission
        Permission::ReadOnly,    // snapshot_permission
        Permission::ReadOnly,    // security_permission
        Permission::ReadOnly     // keyvalue_permission
    };

    // Create an expired token
    RbacToken expiredToken = tokenManager.newToken(
        3,                              // user_id
        expiredPerms.system_permission, // system_permission
        expiredPerms.space_permission,  // space_permission
        expiredPerms.version_permission, // version_permission
        expiredPerms.vector_permission,  // vector_permission
        expiredPerms.search_permission,  // search_permission
        expiredPerms.snapshot_permission, // snapshot_permission
        expiredPerms.security_permission, // security_permission
        expiredPerms.keyvalue_permission, // keyvalue_permission
        1                                // expireDays
    );

    // Manually set to expired 10 seconds ago
    expiredToken.expire_time_utc = std::time(nullptr) - 10;
    tokenManager.updateToken(expiredToken); // Update the expiration time

    // Define permissions for valid token
    Permissions validPerms = {
        Permission::ReadWrite,   // system_permission
        Permission::ReadWrite,   // space_permission
        Permission::ReadWrite,   // version_permission
        Permission::ReadWrite,   // vector_permission
        Permission::ReadWrite,   // search_permission
        Permission::ReadWrite,   // snapshot_permission
        Permission::ReadWrite,   // security_permission
        Permission::ReadWrite    // keyvalue_permission
    };

    // Create a non-expired token
    RbacToken validToken = tokenManager.newToken(
        4,                              // space_id
        validPerms.system_permission,   // system_permission
        validPerms.space_permission,    // space_permission
        validPerms.version_permission,  // version_permission
        validPerms.vector_permission,   // vector_permission
        validPerms.search_permission,   // search_permission
        validPerms.snapshot_permission, // snapshot_permission
        validPerms.security_permission, // security_permission
        validPerms.keyvalue_permission, // keyvalue_permission
        10                              // expireDays
    );

    // Delete all expired tokens
    tokenManager.deleteAllExpireTokens();

    // Check the expired token is deleted
    EXPECT_THROW(tokenManager.getTokenByToken(expiredToken.token), std::runtime_error);

    // Check the non-expired token still exists
    RbacToken retrieved = tokenManager.getTokenByToken(validToken.token);
    EXPECT_EQ(retrieved.space_id, 4);

    // Verify all permissions of the valid token
    EXPECT_EQ(retrieved.system_permission, validPerms.system_permission);
    EXPECT_EQ(retrieved.space_permission, validPerms.space_permission);
    EXPECT_EQ(retrieved.version_permission, validPerms.version_permission);
    EXPECT_EQ(retrieved.vector_permission, validPerms.vector_permission);
    EXPECT_EQ(retrieved.search_permission, validPerms.search_permission);
    EXPECT_EQ(retrieved.snapshot_permission, validPerms.snapshot_permission);
    EXPECT_EQ(retrieved.security_permission, validPerms.security_permission);
    EXPECT_EQ(retrieved.keyvalue_permission, validPerms.keyvalue_permission);
}

// Additional Test: Update Token Permissions
TEST_F(RbacTokenManagerTest, UpdateTokenPermissions) {
    RbacTokenManager& tokenManager = RbacTokenManager::getInstance();

    // Define initial permissions
    Permissions initialPerms = {
        Permission::ReadOnly,    // system_permission
        Permission::ReadOnly,    // space_permission
        Permission::ReadOnly,    // version_permission
        Permission::ReadOnly,    // vector_permission
        Permission::ReadOnly,    // search_permission
        Permission::ReadOnly,    // snapshot_permission
        Permission::ReadOnly,    // security_permission
        Permission::ReadOnly     // keyvalue_permission
    };

    // Create a new token with initial permissions
    RbacToken token = tokenManager.newToken(
        5,                              // user_id
        initialPerms.system_permission, // system_permission
        initialPerms.space_permission,  // space_permission
        initialPerms.version_permission, // version_permission
        initialPerms.vector_permission,  // vector_permission
        initialPerms.search_permission,  // search_permission
        initialPerms.snapshot_permission, // snapshot_permission
        initialPerms.security_permission, // security_permission
        initialPerms.keyvalue_permission, // keyvalue_permission
        expireDays                       // expireDays
    );

    // Verify initial permissions
    EXPECT_EQ(token.system_permission, initialPerms.system_permission);
    EXPECT_EQ(token.space_permission, initialPerms.space_permission);
    EXPECT_EQ(token.version_permission, initialPerms.version_permission);
    EXPECT_EQ(token.vector_permission, initialPerms.vector_permission);
    EXPECT_EQ(token.search_permission, initialPerms.search_permission);
    EXPECT_EQ(token.snapshot_permission, initialPerms.snapshot_permission);
    EXPECT_EQ(token.security_permission, initialPerms.security_permission);
    EXPECT_EQ(token.keyvalue_permission, initialPerms.keyvalue_permission);

    // Define updated permissions
    Permissions updatedPerms = {
        Permission::ReadWrite,   // system_permission
        Permission::ReadWrite,   // space_permission
        Permission::ReadWrite,   // version_permission
        Permission::ReadWrite,   // vector_permission
        Permission::ReadWrite,   // search_permission
        Permission::ReadWrite,   // snapshot_permission
        Permission::ReadWrite,   // security_permission
        Permission::ReadWrite    // keyvalue_permission
    };

    // Update the token's permissions
    token.system_permission = updatedPerms.system_permission;
    token.space_permission = updatedPerms.space_permission;
    token.version_permission = updatedPerms.version_permission;
    token.vector_permission = updatedPerms.vector_permission;
    token.search_permission = updatedPerms.search_permission;
    token.snapshot_permission = updatedPerms.snapshot_permission;
    token.security_permission = updatedPerms.security_permission;
    token.keyvalue_permission = updatedPerms.keyvalue_permission;

    // Update the token in the database
    tokenManager.updateToken(token);

    // Retrieve the updated token
    RbacToken updatedToken = tokenManager.getTokenByToken(token.token);

    // Verify updated permissions
    EXPECT_EQ(updatedToken.system_permission, updatedPerms.system_permission);
    EXPECT_EQ(updatedToken.space_permission, updatedPerms.space_permission);
    EXPECT_EQ(updatedToken.version_permission, updatedPerms.version_permission);
    EXPECT_EQ(updatedToken.vector_permission, updatedPerms.vector_permission);
    EXPECT_EQ(updatedToken.search_permission, updatedPerms.search_permission);
    EXPECT_EQ(updatedToken.snapshot_permission, updatedPerms.snapshot_permission);
    EXPECT_EQ(updatedToken.security_permission, updatedPerms.security_permission);
    EXPECT_EQ(updatedToken.keyvalue_permission, updatedPerms.keyvalue_permission);
}

// Additional Test: Retrieve All Tokens
TEST_F(RbacTokenManagerTest, GetAllTokens) {
    RbacTokenManager& tokenManager = RbacTokenManager::getInstance();

    // Define permissions for first token
    Permissions perms1 = {
        Permission::ReadWrite,   // system_permission
        Permission::ReadOnly,    // space_permission
        Permission::ReadOnly,    // version_permission
        Permission::ReadOnly,    // vector_permission
        Permission::ReadWrite,   // search_permission
        Permission::ReadOnly,    // snapshot_permission
        Permission::ReadWrite,   // security_permission
        Permission::ReadOnly     // keyvalue_permission
    };

    // Define permissions for second token
    Permissions perms2 = {
        Permission::ReadOnly,    // system_permission
        Permission::ReadWrite,   // space_permission
        Permission::ReadWrite,   // version_permission
        Permission::ReadWrite,   // vector_permission
        Permission::ReadOnly,    // search_permission
        Permission::ReadWrite,   // snapshot_permission
        Permission::ReadOnly,    // security_permission
        Permission::ReadWrite    // keyvalue_permission
    };

    // Create first token
    RbacToken token1 = tokenManager.newToken(
        6,                              // space_id
        perms1.system_permission,        // system_permission
        perms1.space_permission,         // space_permission
        perms1.version_permission,       // version_permission
        perms1.vector_permission,        // vector_permission
        perms1.search_permission,        // search_permission
        perms1.snapshot_permission,      // snapshot_permission
        perms1.security_permission,      // security_permission
        perms1.keyvalue_permission,      // keyvalue_permission
        expireDays                       // expireDays
    );

    // Create second token
    RbacToken token2 = tokenManager.newToken(
        7,                              // space_id
        perms2.system_permission,        // system_permission
        perms2.space_permission,         // space_permission
        perms2.version_permission,       // version_permission
        perms2.vector_permission,        // vector_permission
        perms2.search_permission,        // search_permission
        perms2.snapshot_permission,      // snapshot_permission
        perms2.security_permission,      // security_permission
        perms2.keyvalue_permission,      // keyvalue_permission
        expireDays                       // expireDays
    );

    // Retrieve all tokens
    std::vector<RbacToken> allTokens = tokenManager.getAllTokens();

    // Verify the number of tokens
    EXPECT_EQ(allTokens.size(), 2);

    // Verify first token's details
    auto it1 = std::find_if(allTokens.begin(), allTokens.end(), [&](const RbacToken& t) {
        return t.id == token1.id;
    });
    ASSERT_NE(it1, allTokens.end());
    EXPECT_EQ(it1->space_id, 6);
    EXPECT_EQ(it1->system_permission, perms1.system_permission);
    EXPECT_EQ(it1->space_permission, perms1.space_permission);
    EXPECT_EQ(it1->version_permission, perms1.version_permission);
    EXPECT_EQ(it1->vector_permission, perms1.vector_permission);
    EXPECT_EQ(it1->search_permission, perms1.search_permission);
    EXPECT_EQ(it1->snapshot_permission, perms1.snapshot_permission);
    EXPECT_EQ(it1->security_permission, perms1.security_permission);
    EXPECT_EQ(it1->keyvalue_permission, perms1.keyvalue_permission);

    // Verify second token's details
    auto it2 = std::find_if(allTokens.begin(), allTokens.end(), [&](const RbacToken& t) {
        return t.id == token2.id;
    });
    ASSERT_NE(it2, allTokens.end());
    EXPECT_EQ(it2->space_id, 7);
    EXPECT_EQ(it2->system_permission, perms2.system_permission);
    EXPECT_EQ(it2->space_permission, perms2.space_permission);
    EXPECT_EQ(it2->version_permission, perms2.version_permission);
    EXPECT_EQ(it2->vector_permission, perms2.vector_permission);
    EXPECT_EQ(it2->search_permission, perms2.search_permission);
    EXPECT_EQ(it2->snapshot_permission, perms2.snapshot_permission);
    EXPECT_EQ(it2->security_permission, perms2.security_permission);
    EXPECT_EQ(it2->keyvalue_permission, perms2.keyvalue_permission);
}
