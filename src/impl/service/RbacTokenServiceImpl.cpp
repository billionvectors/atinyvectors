#include "service/RbacTokenService.hpp"
#include "RbacToken.hpp"
#include "IdCache.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace atinyvectors {
namespace service {

namespace {

int permissionToServiceValue(Permission permission) {
    switch (permission) {
        case Permission::ReadOnly: return 1;
        case Permission::ReadWrite: return 2;
        default: return 0;
    }
}

} // anonymous namespace

int RbacTokenServiceManager::getSystemPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToServiceValue(rbacToken.system_permission);
    } catch (const std::exception& e) {
        spdlog::error("Error in getSystemPermission: {}", e.what());
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenServiceManager::getSpacePermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToServiceValue(rbacToken.space_permission);
    } catch (const std::exception& e) {
        spdlog::error("Error in getSpacePermission: {}", e.what());
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenServiceManager::getVersionPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToServiceValue(rbacToken.version_permission);
    } catch (const std::exception& e) {
        spdlog::error("Error in getVersionPermission: {}", e.what());
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenServiceManager::getVectorPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToServiceValue(rbacToken.vector_permission);
    } catch (const std::exception& e) {
        spdlog::error("Error in getVectorPermission: {}", e.what());
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenServiceManager::getSnapshotPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToServiceValue(rbacToken.snapshot_permission);
    } catch (const std::exception& e) {
        spdlog::error("Error in getSnapshotPermission: {}", e.what());
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenServiceManager::getSearchPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToServiceValue(rbacToken.search_permission);
    } catch (const std::exception& e) {
        spdlog::error("Error in getSearchPermission: {}", e.what());
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenServiceManager::getSecurityPermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToServiceValue(rbacToken.security_permission);
    } catch (const std::exception& e) {
        spdlog::error("Error in getSecurityPermission: {}", e.what());
        throw std::runtime_error("Token not found or expired");
    }
}

int RbacTokenServiceManager::getKeyvaluePermission(const std::string& token) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        return permissionToServiceValue(rbacToken.keyvalue_permission);
    } catch (const std::exception& e) {
        spdlog::error("Error in getKeyvaluePermission: {}", e.what());
        throw std::runtime_error("Token not found or expired");
    }
}

nlohmann::json RbacTokenServiceManager::listTokens() {
    try {
        auto tokens = RbacTokenManager::getInstance().getAllTokens();
        nlohmann::json result = nlohmann::json::array();
        for (const auto& token : tokens) {
            nlohmann::json tokenJson;
            tokenJson["id"] = token.id;
            tokenJson["space_id"] = token.space_id;
            tokenJson["token"] = token.token;
            tokenJson["expire_time_utc"] = token.expire_time_utc;
            tokenJson["system"] = permissionToServiceValue(token.system_permission);
            tokenJson["space"] = permissionToServiceValue(token.space_permission);
            tokenJson["version"] = permissionToServiceValue(token.version_permission);
            tokenJson["vector"] = permissionToServiceValue(token.vector_permission);
            tokenJson["search"] = permissionToServiceValue(token.search_permission);
            tokenJson["snapshot"] = permissionToServiceValue(token.snapshot_permission);
            tokenJson["security"] = permissionToServiceValue(token.security_permission);
            tokenJson["keyvalue"] = permissionToServiceValue(token.keyvalue_permission);

            result.push_back(tokenJson);
        }
        return result;
    } catch (const std::exception& e) {
        spdlog::error("Error in listTokens: {}", e.what());
        throw;
    }
}

void RbacTokenServiceManager::deleteToken(const std::string& token) {
    try {
        RbacTokenManager::getInstance().deleteByToken(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in deleteToken: {}", e.what());
        throw;
    }
}

void RbacTokenServiceManager::updateToken(const std::string& token, const std::string& jsonStr) {
    try {
        auto rbacToken = IdCache::getInstance().getRbacToken(token);
        nlohmann::json parsedJson = nlohmann::json::parse(jsonStr);

        if (parsedJson.contains("expire_time_utc")) {
            rbacToken.expire_time_utc = parsedJson["expire_time_utc"];
        }

        if (parsedJson.contains("system")) {
            rbacToken.system_permission = static_cast<Permission>(parsedJson["system"].get<int>());
        }
        if (parsedJson.contains("space")) {
            rbacToken.space_permission = static_cast<Permission>(parsedJson["space"].get<int>());
        }
        if (parsedJson.contains("version")) {
            rbacToken.version_permission = static_cast<Permission>(parsedJson["version"].get<int>());
        }
        if (parsedJson.contains("vector")) {
            rbacToken.vector_permission = static_cast<Permission>(parsedJson["vector"].get<int>());
        }
        if (parsedJson.contains("search")) {
            rbacToken.search_permission = static_cast<Permission>(parsedJson["search"].get<int>());
        }
        if (parsedJson.contains("snapshot")) {
            rbacToken.snapshot_permission = static_cast<Permission>(parsedJson["snapshot"].get<int>());
        }
        if (parsedJson.contains("security")) {
            rbacToken.security_permission = static_cast<Permission>(parsedJson["security"].get<int>());
        }
        if (parsedJson.contains("keyvalue")) {
            rbacToken.keyvalue_permission = static_cast<Permission>(parsedJson["keyvalue"].get<int>());
        }

        RbacTokenManager::getInstance().updateToken(rbacToken);

        IdCache::getInstance().getRbacToken(token); // Update cache
    } catch (const std::exception& e) {
        spdlog::error("Error in updateToken: {}", e.what());
        throw;
    }
}

nlohmann::json RbacTokenServiceManager::newToken(const std::string& jsonStr, const std::string& token) {
    try {
        nlohmann::json parsedJson = nlohmann::json::parse(jsonStr);

        int space_id = parsedJson.contains("space_id") ? parsedJson["space_id"].get<int>() : 0;
        Permission system_perm = static_cast<Permission>(parsedJson.contains("system") ? parsedJson["system"].get<int>() : 0);
        Permission space_perm = static_cast<Permission>(parsedJson.contains("space") ? parsedJson["space"].get<int>() : 0);
        Permission version_perm = static_cast<Permission>(parsedJson.contains("version") ? parsedJson["version"].get<int>() : 0);
        Permission vector_perm = static_cast<Permission>(parsedJson.contains("vector") ? parsedJson["vector"].get<int>() : 0);
        Permission search_perm = static_cast<Permission>(parsedJson.contains("search") ? parsedJson["search"].get<int>() : 0);       // Added
        Permission snapshot_perm = static_cast<Permission>(parsedJson.contains("snapshot") ? parsedJson["snapshot"].get<int>() : 0);
        Permission security_perm = static_cast<Permission>(parsedJson.contains("security") ? parsedJson["security"].get<int>() : 0);   // Added
        Permission keyvalue_perm = static_cast<Permission>(parsedJson.contains("keyvalue") ? parsedJson["keyvalue"].get<int>() : 0); // Added

        int expire_days = parsedJson.contains("expire_days") ? parsedJson["expire_days"].get<int>() : 0;

        auto newToken = RbacTokenManager::getInstance().newToken(
            space_id, 
            system_perm, 
            space_perm, 
            version_perm, 
            vector_perm, 
            search_perm,
            snapshot_perm, 
            security_perm,
            keyvalue_perm,
            expire_days, 
            token);

        nlohmann::json result;
        result["token"] = newToken.token;
        result["expire_time_utc"] = newToken.expire_time_utc;

        return result;
    } catch (const std::exception& e) {
        spdlog::error("Error in newToken: {}", e.what());
        throw;
    }
}

std::string RbacTokenServiceManager::generateJWTToken(int expireDays) {
    return RbacTokenManager::generateJWTToken(expireDays);
}

} // namespace dto
} // namespace atinyvectors
