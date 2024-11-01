#include "atinyvectors_c_api.h"
#include "service/VectorService.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

// C API for VectorServiceManager
VectorServiceManager* atv_vector_service_manager_new() {
    return reinterpret_cast<VectorServiceManager*>(new atinyvectors::service::VectorServiceManager());
}

void atv_vector_service_manager_free(VectorServiceManager* manager) {
    delete reinterpret_cast<atinyvectors::service::VectorServiceManager*>(manager);
}

void atv_vector_service_upsert(VectorServiceManager* manager, const char* spaceName, int versionId, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::VectorServiceManager*>(manager);
        cppManager->upsert(spaceName, versionId, jsonStr);
    } catch (const nlohmann::json::exception& e) {
        atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_vector_service_get_vectors_by_version_id(VectorServiceManager* manager, const char* spaceName, int versionId, int start, int limit) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::VectorServiceManager*>(manager);
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
