#include "atinyvectors_c_api.h"
#include "service/SpaceService.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

SpaceServiceManager* atv_space_service_manager_new() {
    return reinterpret_cast<SpaceServiceManager*>(new atinyvectors::service::SpaceServiceManager());
}

void atv_space_service_manager_free(SpaceServiceManager* manager) {
    delete reinterpret_cast<atinyvectors::service::SpaceServiceManager*>(manager);
}

void atv_space_service_create_space(SpaceServiceManager* manager, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SpaceServiceManager*>(manager);
        cppManager->createSpace(jsonStr);
    } catch (const nlohmann::json::exception& e) {
        atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

void atv_space_service_update_space(SpaceServiceManager* manager, const char* spaceName, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SpaceServiceManager*>(manager);
        cppManager->updateSpace(spaceName, jsonStr);
    } catch (const nlohmann::json::exception& e) {
        atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

void atv_space_service_delete_space(SpaceServiceManager* manager, const char* spaceName, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SpaceServiceManager*>(manager);
        cppManager->deleteSpace(spaceName, jsonStr);
    } catch (const nlohmann::json::exception& e) {
        atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_space_service_get_by_space_id(SpaceServiceManager* manager, int spaceId) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SpaceServiceManager*>(manager);
        nlohmann::json result = cppManager->getBySpaceId(spaceId);
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

char* atv_space_service_get_by_space_name(SpaceServiceManager* manager, const char* spaceName) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SpaceServiceManager*>(manager);
        nlohmann::json result = cppManager->getBySpaceName(spaceName);
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

char* atv_space_service_get_lists(SpaceServiceManager* manager) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SpaceServiceManager*>(manager);
        nlohmann::json result = cppManager->getLists();
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
