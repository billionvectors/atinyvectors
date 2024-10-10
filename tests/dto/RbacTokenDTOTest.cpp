#include <gtest/gtest.h>
#include "dto/RbacTokenDTO.hpp"
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
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include <vector>
#include <cmath>
#include <jwt-cpp/jwt.h>

using namespace atinyvectors;
using namespace atinyvectors::dto;
using namespace atinyvectors::algo;
using namespace nlohmann;

// Test Fixture class
class RbacTokenDTOTest : public ::testing::Test {
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
    
    void validateTokenExpiration(const std::string& token, int expectedExpireDays) {
        auto& config = atinyvectors::Config::getInstance();
        std::string jwt_key = config.getJwtTokenKey();

        auto decoded = jwt::decode(token);

        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{jwt_key})
            .with_claim("exp", jwt::claim(jwt::date(std::chrono::system_clock::now() + std::chrono::hours(24 * expectedExpireDays))));

        EXPECT_NO_THROW(verifier.verify(decoded));

        auto expClaim = decoded.get_payload_claim("exp").as_date();
        auto now = std::chrono::system_clock::now();
        auto expectedExpTime = now + std::chrono::hours(24 * expectedExpireDays);

        EXPECT_NEAR(std::chrono::duration_cast<std::chrono::seconds>(expClaim - now).count(),
                    std::chrono::duration_cast<std::chrono::seconds>(expectedExpTime - now).count(),
                    60);
    }
};

TEST_F(RbacTokenDTOTest, GetPermissions) {
    RbacTokenDTOManager dtoManager;
    auto newTokenJson = dtoManager.newToken("{\"space_id\": 1, \"system\": 2, \"space\": 1, \"vector\": 2, \"snapshot\": 1}");

    std::string token = newTokenJson["token"];

    EXPECT_EQ(dtoManager.getSystemPermission(token), 2); // Expecting ReadWrite
    EXPECT_EQ(dtoManager.getSpacePermission(token), 1); // Expecting ReadOnly
    EXPECT_EQ(dtoManager.getVersionPermission(token), 0); // Expecting Denied
    EXPECT_EQ(dtoManager.getVectorPermission(token), 2); // Expecting ReadWrite
    EXPECT_EQ(dtoManager.getSnapshotPermission(token), 1); // Expecting ReadOnly
}

TEST_F(RbacTokenDTOTest, ListTokens) {
    RbacTokenDTOManager dtoManager;
    dtoManager.newToken("{\"space_id\": 1, \"system\": 2, \"space\": 1, \"vector\": 2, \"snapshot\": 1}");
    auto tokens = dtoManager.listTokens();
    EXPECT_GT(tokens.size(), 0); // Ensure at least one token is listed
}

TEST_F(RbacTokenDTOTest, DeleteToken) {
    RbacTokenDTOManager dtoManager;
    auto newTokenJson = dtoManager.newToken("{\"space_id\": 1, \"system\": 2, \"space\": 1, \"vector\": 2, \"snapshot\": 1}");
    std::string token = newTokenJson["token"];
    
    dtoManager.deleteToken(token);
    EXPECT_THROW(dtoManager.getSystemPermission(token), std::runtime_error); // Token should throw an exception when not found
}

TEST_F(RbacTokenDTOTest, UpdateToken) {
    RbacTokenDTOManager dtoManager;
    auto newTokenJson = dtoManager.newToken("{\"space_id\": 1, \"system\": 2, \"space\": 1, \"vector\": 2, \"snapshot\": 1}");
    std::string token = newTokenJson["token"];

    dtoManager.updateToken(token, "{\"expire_time_utc\": 9999999999}");
    EXPECT_GT(dtoManager.getSystemPermission(token), 0); // Ensure token is still valid after update
}

TEST_F(RbacTokenDTOTest, NewTokenWithExistingToken) {
    RbacTokenDTOManager dtoManager;
    std::string existingToken = "existing_token_value";

    // Create a new token with a specific token value
    auto newTokenJson = dtoManager.newToken("{\"space_id\": 2, \"system\": 1, \"space\": 2, \"version\": 1, \"snapshot\": 2}", existingToken.c_str());

    EXPECT_EQ(newTokenJson["token"], existingToken); // Ensure the provided token value is used

    EXPECT_EQ(dtoManager.getSystemPermission(existingToken), 1); // Expecting ReadOnly
    EXPECT_EQ(dtoManager.getSpacePermission(existingToken), 2); // Expecting ReadWrite
    EXPECT_EQ(dtoManager.getVersionPermission(existingToken), 1); // Expecting ReadOnly
    EXPECT_EQ(dtoManager.getVectorPermission(existingToken), 0); // Expecting Denied
    EXPECT_EQ(dtoManager.getSnapshotPermission(existingToken), 2); // Expecting ReadWrite
}

TEST_F(RbacTokenDTOTest, GenerateJWTToken) {
    RbacTokenDTOManager dtoManager;

    std::string jwtTokenDefault = dtoManager.generateJWTToken(0);
    EXPECT_FALSE(jwtTokenDefault.empty());

    // Generate a JWT token with 10 days expiration
    std::string jwtToken10Days = dtoManager.generateJWTToken(10);
    EXPECT_FALSE(jwtToken10Days.empty());

    EXPECT_NE(jwtTokenDefault, jwtToken10Days);

    std::string jwtTokenDefaultExpire = dtoManager.generateJWTToken(0);
    EXPECT_FALSE(jwtTokenDefaultExpire.empty()); 

    validateTokenExpiration(jwtToken10Days, 10); // Check if the token expires in 10 days
}