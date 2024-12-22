#include <gtest/gtest.h>
#include "service/RbacTokenService.hpp"
#include "algo/FaissIndexLRUCache.hpp"
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
using namespace atinyvectors::service;
using namespace atinyvectors::algo;
using namespace nlohmann;

// Test Fixture class
class RbacTokenServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        IdCache::getInstance().clean();

        // clean data
        DatabaseManager::getInstance().reset();
        FaissIndexLRUCache::getInstance().clean();
    }

    void TearDown() override {
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

TEST_F(RbacTokenServiceTest, GetPermissions) {
    RbacTokenServiceManager serviceManager;
    auto newTokenJson = serviceManager.newToken("{\"space_id\": 1, \"system\": 2, \"space\": 1, \"vector\": 2, \"snapshot\": 1}");

    std::string token = newTokenJson["token"];

    EXPECT_EQ(serviceManager.getSystemPermission(token), 2); // Expecting ReadWrite
    EXPECT_EQ(serviceManager.getSpacePermission(token), 1); // Expecting ReadOnly
    EXPECT_EQ(serviceManager.getVersionPermission(token), 0); // Expecting Denied
    EXPECT_EQ(serviceManager.getVectorPermission(token), 2); // Expecting ReadWrite
    EXPECT_EQ(serviceManager.getSnapshotPermission(token), 1); // Expecting ReadOnly
}

TEST_F(RbacTokenServiceTest, ListTokens) {
    RbacTokenServiceManager serviceManager;
    serviceManager.newToken("{\"space_id\": 1, \"system\": 2, \"space\": 1, \"vector\": 2, \"snapshot\": 1}");
    auto tokens = serviceManager.listTokens();
    EXPECT_GT(tokens.size(), 0); // Ensure at least one token is listed
}

TEST_F(RbacTokenServiceTest, DeleteToken) {
    RbacTokenServiceManager serviceManager;
    auto newTokenJson = serviceManager.newToken("{\"space_id\": 1, \"system\": 2, \"space\": 1, \"vector\": 2, \"snapshot\": 1}");
    std::string token = newTokenJson["token"];
    
    serviceManager.deleteToken(token);
    EXPECT_THROW(serviceManager.getSystemPermission(token), std::runtime_error); // Token should throw an exception when not found
}

TEST_F(RbacTokenServiceTest, UpdateToken) {
    RbacTokenServiceManager serviceManager;
    auto newTokenJson = serviceManager.newToken("{\"space_id\": 1, \"system\": 2, \"space\": 1, \"vector\": 2, \"snapshot\": 1}");
    std::string token = newTokenJson["token"];

    serviceManager.updateToken(token, "{\"expire_time_utc\": 9999999999}");
    EXPECT_GT(serviceManager.getSystemPermission(token), 0); // Ensure token is still valid after update
}

TEST_F(RbacTokenServiceTest, NewTokenWithExistingToken) {
    RbacTokenServiceManager serviceManager;
    std::string existingToken = "existing_token_value";

    // Create a new token with a specific token value
    auto newTokenJson = serviceManager.newToken("{\"space_id\": 2, \"system\": 1, \"space\": 2, \"version\": 1, \"snapshot\": 2}", existingToken.c_str());

    EXPECT_EQ(newTokenJson["token"], existingToken); // Ensure the provided token value is used

    EXPECT_EQ(serviceManager.getSystemPermission(existingToken), 1); // Expecting ReadOnly
    EXPECT_EQ(serviceManager.getSpacePermission(existingToken), 2); // Expecting ReadWrite
    EXPECT_EQ(serviceManager.getVersionPermission(existingToken), 1); // Expecting ReadOnly
    EXPECT_EQ(serviceManager.getVectorPermission(existingToken), 0); // Expecting Denied
    EXPECT_EQ(serviceManager.getSnapshotPermission(existingToken), 2); // Expecting ReadWrite
}

TEST_F(RbacTokenServiceTest, GenerateJWTToken) {
    RbacTokenServiceManager serviceManager;

    std::string jwtTokenDefault = serviceManager.generateJWTToken(0);
    EXPECT_FALSE(jwtTokenDefault.empty());

    // Generate a JWT token with 10 days expiration
    std::string jwtToken10Days = serviceManager.generateJWTToken(10);
    EXPECT_FALSE(jwtToken10Days.empty());

    EXPECT_NE(jwtTokenDefault, jwtToken10Days);

    std::string jwtTokenDefaultExpire = serviceManager.generateJWTToken(0);
    EXPECT_FALSE(jwtTokenDefaultExpire.empty()); 

    validateTokenExpiration(jwtToken10Days, 10); // Check if the token expires in 10 days
}