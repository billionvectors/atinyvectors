#include "atinyvectors_c_api.h"
#include "dto/VectorDTO.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

// C API for VectorDTOManager
VectorDTOManager* atv_vector_dto_manager_new() {
    return reinterpret_cast<VectorDTOManager*>(new atinyvectors::dto::VectorDTOManager());
}

void atv_vector_dto_manager_free(VectorDTOManager* manager) {
    delete reinterpret_cast<atinyvectors::dto::VectorDTOManager*>(manager);
}

void atv_vector_dto_upsert(VectorDTOManager* manager, const char* spaceName, int versionId, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::VectorDTOManager*>(manager);
        cppManager->upsert(spaceName, versionId, jsonStr);
    } catch (const nlohmann::json::exception& e) {
        atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_vector_dto_get_vectors_by_version_id(VectorDTOManager* manager, const char* spaceName, int versionId, int start, int limit) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::VectorDTOManager*>(manager);
        nlohmann::json result = cppManager->getVectorsByVersionId(spaceName, versionId, start, limit);
        
        // Allocate memory and return JSON string
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
