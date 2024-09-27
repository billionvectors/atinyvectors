#include "atinyvectors_c_api.h"
#include "dto/SpaceDTO.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

SpaceDTOManager* atv_space_dto_manager_new() {
    return reinterpret_cast<SpaceDTOManager*>(new atinyvectors::dto::SpaceDTOManager());
}

void atv_space_dto_manager_free(SpaceDTOManager* manager) {
    delete reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
}

void atv_space_dto_create_space(SpaceDTOManager* manager, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
        cppManager->createSpace(jsonStr);
    } catch (const nlohmann::json::exception& e) {
        atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

void atv_space_dto_delete_space(SpaceDTOManager* manager, const char* spaceName, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
        cppManager->deleteSpace(spaceName, jsonStr);
    } catch (const nlohmann::json::exception& e) {
        atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_space_dto_get_by_space_id(SpaceDTOManager* manager, int spaceId) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
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

char* atv_space_dto_get_by_space_name(SpaceDTOManager* manager, const char* spaceName) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
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

char* atv_space_dto_get_lists(SpaceDTOManager* manager) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
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
