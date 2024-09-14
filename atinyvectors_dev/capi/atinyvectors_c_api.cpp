#include "atinyvectors_c_api.h"
#include "dto/SearchDTO.hpp"
#include "dto/SpaceDTO.hpp"
#include "dto/VersionDTO.hpp"
#include "dto/VectorDTO.hpp"
#include <cstring>
#include <iostream>

// C API for SearchDTOManager
SearchDTOManager* search_dto_manager_new() {
    return reinterpret_cast<SearchDTOManager*>(new atinyvectors::dto::SearchDTOManager());
}

void search_dto_manager_free(SearchDTOManager* manager) {
    delete reinterpret_cast<atinyvectors::dto::SearchDTOManager*>(manager);
}

char* search_dto_search(SearchDTOManager* manager, const char* spaceName, const char* queryJsonStr, size_t k) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::SearchDTOManager*>(manager);
    std::vector<std::pair<float, hnswlib::labeltype>> results = cppManager->search(spaceName, queryJsonStr, k);
    nlohmann::json jsonResult = cppManager->extractSearchResultsToJson(results);
    
    // Allocate memory and return JSON string
    std::string jsonString = jsonResult.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

// C API for SpaceDTOManager
SpaceDTOManager* space_dto_manager_new() {
    return reinterpret_cast<SpaceDTOManager*>(new atinyvectors::dto::SpaceDTOManager());
}

void space_dto_manager_free(SpaceDTOManager* manager) {
    delete reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
}

void space_dto_create_space(SpaceDTOManager* manager, const char* jsonStr) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
    cppManager->createSpace(jsonStr);
}

char* space_dto_get_by_space_id(SpaceDTOManager* manager, int spaceId) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
    nlohmann::json result = cppManager->getBySpaceId(spaceId);
    
    // Allocate memory and return JSON string
    std::string jsonString = result.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

char* space_dto_get_by_space_name(SpaceDTOManager* manager, const char* spaceName) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
    nlohmann::json result = cppManager->getBySpaceName(spaceName);
    
    // Allocate memory and return JSON string
    std::string jsonString = result.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

char* space_dto_get_lists(SpaceDTOManager* manager) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::SpaceDTOManager*>(manager);
    nlohmann::json result = cppManager->getLists();
    
    // Allocate memory and return JSON string
    std::string jsonString = result.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

// C API for VersionDTOManager
VersionDTOManager* version_dto_manager_new() {
    return reinterpret_cast<VersionDTOManager*>(new atinyvectors::dto::VersionDTOManager());
}

void version_dto_manager_free(VersionDTOManager* manager) {
    delete reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
}

void version_dto_create_version(VersionDTOManager* manager, const char* spaceName, const char* jsonStr) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
    cppManager->createVersion(spaceName, jsonStr);
}

char* version_dto_get_by_version_id(VersionDTOManager* manager, const char* spaceName, int versionId) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
    nlohmann::json result = cppManager->getByVersionId(spaceName, versionId);

    std::string jsonString = result.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

char* version_dto_get_by_version_name(VersionDTOManager* manager, const char* spaceName, const char* versionName) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
    nlohmann::json result = cppManager->getByVersionName(spaceName, versionName);

    std::string jsonString = result.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

char* version_dto_get_default_version(VersionDTOManager* manager, const char* spaceName) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
    nlohmann::json result = cppManager->getDefaultVersion(spaceName);

    std::string jsonString = result.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

char* version_dto_get_lists(VersionDTOManager* manager, const char* spaceName) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::VersionDTOManager*>(manager);
    nlohmann::json result = cppManager->getLists(spaceName);

    std::string jsonString = result.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

// C API for VectorDTOManager
VectorDTOManager* vector_dto_manager_new() {
    return reinterpret_cast<VectorDTOManager*>(new atinyvectors::dto::VectorDTOManager());
}

void vector_dto_manager_free(VectorDTOManager* manager) {
    delete reinterpret_cast<atinyvectors::dto::VectorDTOManager*>(manager);
}

void vector_dto_upsert(VectorDTOManager* manager, const char* spaceName, int versionId, const char* jsonStr) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::VectorDTOManager*>(manager);
    cppManager->upsert(spaceName, versionId, jsonStr);
}

char* vector_dto_get_vectors_by_version_id(VectorDTOManager* manager, int versionId) {
    auto* cppManager = reinterpret_cast<atinyvectors::dto::VectorDTOManager*>(manager);
    nlohmann::json result = cppManager->getVectorsByVersionId(versionId);
    
    // Allocate memory and return JSON string
    std::string jsonString = result.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

// Function to free JSON string memory
void free_json_string(char* jsonStr) {
    free(jsonStr);
}
