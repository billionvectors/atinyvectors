#include "dto/RbacTokenDTO.hpp"
#include "RbacToken.hpp"
#include "IdCache.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace atinyvectors {
namespace dto {

namespace {

int permissionToDTOValue(Permission permission) {
    switch (permission) {
        case Permission::ReadOnly: return 1;  // ReadOnly should return 1
        case Permission::ReadWrite: return 2; // ReadWrite should return 2
        default: return 0; // Any other case should return 0 (Denied)
    }
}

} // anonymous namespace

// Permission methods
int RbacTokenDTOManager::getSystemPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToDTOValue(rbacToken.system_permission);
    } catch (const std::exception& e) {
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenDTOManager::getSpacePermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToDTOValue(rbacToken.space_permission);
    } catch (const std::exception& e) {
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenDTOManager::getVersionPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToDTOValue(rbacToken.version_permission);
    } catch (const std::exception& e) {
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenDTOManager::getVectorPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToDTOValue(rbacToken.vector_permission);
    } catch (const std::exception& e) {
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenDTOManager::getSnapshotPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToDTOValue(rbacToken.snapshot_permission);
    } catch (const std::exception& e) {
        throw std::runtime_error("Token not found or expired");
    }
}

nlohmann::json RbacTokenDTOManager::listTokens() {
    try {
        auto tokens = RbacTokenManager::getInstance().getAllTokens();
        nlohmann::json result = nlohmann::json::array();
        for (const auto& token : tokens) {
            nlohmann::json tokenJson;
            tokenJson["id"] = token.id;
            tokenJson["user_id"] = token.user_id;
            tokenJson["token"] = token.token;
            tokenJson["expire_time_utc"] = token.expire_time_utc;
            tokenJson["system"] = static_cast<int>(token.system_permission);
            tokenJson["space"] = static_cast<int>(token.space_permission);
            tokenJson["version"] = static_cast<int>(token.version_permission);
            tokenJson["vector"] = static_cast<int>(token.vector_permission);
            tokenJson["snapshot"] = static_cast<int>(token.snapshot_permission);

            result.push_back(tokenJson);
        }
        return result;
    } catch (const std::exception& e) {
        throw;
    }
}

void RbacTokenDTOManager::deleteToken(const std::string& token) {
    try {
        RbacTokenManager::getInstance().deleteByToken(token);
    } catch (const std::exception& e) {
        throw;
    }
}

void RbacTokenDTOManager::updateToken(const std::string& token, const std::string& jsonStr) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        nlohmann::json parsedJson = nlohmann::json::parse(jsonStr);

        if (parsedJson.contains("expire_time_utc")) {
            rbacToken.expire_time_utc = parsedJson["expire_time_utc"];
        }

        RbacTokenManager::getInstance().updateToken(rbacToken);

        // Ensure the cache is updated after token modification
        IdCache::getInstance().getRbacToken(token); // Update cache
    } catch (const std::exception& e) {
        throw;
    }
}

nlohmann::json RbacTokenDTOManager::newToken(const std::string& jsonStr, const std::string& token) {
    try {
        nlohmann::json parsedJson = nlohmann::json::parse(jsonStr);

        // default is denied
        int user_id = parsedJson.contains("user_id") ? parsedJson["user_id"].get<int>() : 0;
        Permission system_perm = static_cast<Permission>(parsedJson.contains("system") ? parsedJson["system"].get<int>() : 0);
        Permission space_perm = static_cast<Permission>(parsedJson.contains("space") ? parsedJson["space"].get<int>() : 0);
        Permission version_perm = static_cast<Permission>(parsedJson.contains("version") ? parsedJson["version"].get<int>() : 0);
        Permission vector_perm = static_cast<Permission>(parsedJson.contains("vector") ? parsedJson["vector"].get<int>() : 0);
        Permission snapshot_perm = static_cast<Permission>(parsedJson.contains("snapshot") ? parsedJson["snapshot"].get<int>() : 0);
        
        // default value is expire days value in config
        int expire_days = parsedJson.contains("expire_days") ? parsedJson["expire_days"].get<int>() : 0;

        auto newToken = RbacTokenManager::getInstance().newToken(
            user_id, system_perm, space_perm, version_perm, vector_perm, snapshot_perm, expire_days, token);

        nlohmann::json result;
        result["token"] = newToken.token;
        result["expire_time_utc"] = newToken.expire_time_utc;

        return result;
    } catch (const std::exception& e) {
        throw;
    }
}


// Added generateToken function in RbacTokenDTOManager
std::string RbacTokenDTOManager::generateJWTToken(int expireDays) {
    return RbacTokenManager::generateJWTToken(expireDays);
}

} // namespace dto
} // namespace atinyvectors