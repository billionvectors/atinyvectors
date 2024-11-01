#include "atinyvectors_c_api.h"
#include "service/RbacTokenService.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

// C API for RbacTokenServiceManager
RbacTokenServiceManager* atv_rbac_token_service_manager_new() {
    return reinterpret_cast<RbacTokenServiceManager*>(new atinyvectors::service::RbacTokenServiceManager());
}

void atv_rbac_token_service_manager_free(RbacTokenServiceManager* manager) {
    delete reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
}

int atv_rbac_token_get_system_permission(RbacTokenServiceManager* manager, const char* token) {
    auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
    try {
        return cppManager->getSystemPermission(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in get_system_permission: {}", e.what());
        return 0;  // Return 0 if there is an exception (token expired or not found)
    }
}

int atv_rbac_token_get_space_permission(RbacTokenServiceManager* manager, const char* token) {
    auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
    try {
        return cppManager->getSpacePermission(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in get_space_permission: {}", e.what());
        return 0;
    }
}

int atv_rbac_token_get_version_permission(RbacTokenServiceManager* manager, const char* token) {
    auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
    try {
        return cppManager->getVersionPermission(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in get_version_permission: {}", e.what());
        return 0;
    }
}

int atv_rbac_token_get_vector_permission(RbacTokenServiceManager* manager, const char* token) {
    auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
    try {
        return cppManager->getVectorPermission(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in get_vector_permission: {}", e.what());
        return 0;
    }
}

int atv_rbac_token_get_snapshot_permission(RbacTokenServiceManager* manager, const char* token) {
    auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
    try {
        return cppManager->getSnapshotPermission(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in get_snapshot_permission: {}", e.what());
        return 0;
    }
}

int atv_rbac_token_get_search_permission(RbacTokenServiceManager* manager, const char* token) {
    auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
    try {
        return cppManager->getSearchPermission(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in get_search_permission: {}", e.what());
        return 0;  // Return 0 if there is an exception (token expired or not found)
    }
}

int atv_rbac_token_get_security_permission(RbacTokenServiceManager* manager, const char* token) {
    auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
    try {
        return cppManager->getSecurityPermission(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in get_security_permission: {}", e.what());
        return 0;  // Return 0 if there is an exception (token expired or not found)
    }
}

int atv_rbac_token_get_keyvalue_permission(RbacTokenServiceManager* manager, const char* token) {
    auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
    try {
        return cppManager->getKeyvaluePermission(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in get_keyvalue_permission: {}", e.what());
        return 0;  // Return 0 if there is an exception (token expired or not found)
    }
}

char* atv_rbac_token_new_token(RbacTokenServiceManager* manager, const char* jsonStr, const char* token) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);

        // Check if token is provided; if not, pass an empty string
        std::string providedToken = (token != nullptr) ? token : "";
        nlohmann::json result = cppManager->newToken(jsonStr, providedToken);

        // Convert JSON result to a C string
        std::string jsonString = result.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        if (!resultCStr) {
            return atv_create_error_json(ATVErrorCode::MEMORY_ALLOCATION_ERROR, "Failed to allocate memory");
        }
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const nlohmann::json::exception& e) {
        return atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_rbac_token_generate_jwt_token(RbacTokenServiceManager* manager, int expireDays) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
        std::string newToken = cppManager->generateJWTToken(expireDays);

        char* resultCStr = (char*)malloc(newToken.size() + 1);
        if (!resultCStr) {
            return atv_create_error_json(ATVErrorCode::MEMORY_ALLOCATION_ERROR, "Failed to allocate memory");
        }
        std::strcpy(resultCStr, newToken.c_str());
        return resultCStr;
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_rbac_token_list_tokens(RbacTokenServiceManager* manager) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
        nlohmann::json result = cppManager->listTokens();

        // Convert JSON result to a C string
        std::string jsonString = result.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        if (!resultCStr) {
            return atv_create_error_json(ATVErrorCode::MEMORY_ALLOCATION_ERROR, "Failed to allocate memory");
        }
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const nlohmann::json::exception& e) {
        return atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

void atv_rbac_token_delete_token(RbacTokenServiceManager* manager, const char* token) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
        cppManager->deleteToken(token);
    } catch (const std::exception& e) {
        spdlog::error("Error in delete_token: {}", e.what());
        // Depending on C API design, you might want to handle the error differently
    }
}

void atv_rbac_token_update_token(RbacTokenServiceManager* manager, const char* token, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::RbacTokenServiceManager*>(manager);
        cppManager->updateToken(token, jsonStr);
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Error in update_token (JSON): {}", e.what());
        // Handle JSON parsing error
    } catch (const std::exception& e) {
        spdlog::error("Error in update_token: {}", e.what());
        // Handle other exceptions
    }
}