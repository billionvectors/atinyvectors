#include "atinyvectors_c_api.h"
#include "dto/VersionDTO.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

// C API for VersionDTOManager
VersionDTOManager* atv_version_dto_manager_new() {
    return reinterpret_cast<VersionDTOManager*>(new atinyvectors::dto::VersionDTOManager());
}

void atv_version_dto_manager_free(VersionDTOManager* manager) {
    delete reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
}

void atv_version_dto_create_version(VersionDTOManager* manager, const char* spaceName, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
        cppManager->createVersion(spaceName, jsonStr);
    } catch (const nlohmann::json::exception& e) {
        atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_version_dto_get_by_version_id(VersionDTOManager* manager, const char* spaceName, int versionId) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
        nlohmann::json result = cppManager->getByVersionId(spaceName, versionId);

        std::string jsonString = result.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const nlohmann::json::exception& e) {
        return atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_version_dto_get_by_version_name(VersionDTOManager* manager, const char* spaceName, const char* versionName) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
        nlohmann::json result = cppManager->getByVersionName(spaceName, versionName);

        std::string jsonString = result.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const nlohmann::json::exception& e) {
        return atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_version_dto_get_default_version(VersionDTOManager* manager, const char* spaceName) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
        nlohmann::json result = cppManager->getDefaultVersion(spaceName);

        std::string jsonString = result.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const nlohmann::json::exception& e) {
        return atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_version_dto_get_lists(VersionDTOManager* manager, const char* spaceName) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
        nlohmann::json result = cppManager->getLists(spaceName);

        std::string jsonString = result.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const nlohmann::json::exception& e) {
        return atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}
