#include "atinyvectors_c_api.h"
#include "IdCache.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using namespace atinyvectors;

IdCacheManager* atv_id_cache_manager_new() {
    return reinterpret_cast<IdCacheManager*>(&IdCache::getInstance());
}

void atv_id_cache_manager_free(IdCacheManager* manager) {
    // No explicit deallocation needed as IdCache is a singleton
}

int atv_id_cache_get_version_id(IdCacheManager* manager, const char* spaceName, int versionUniqueId) {
    try {
        auto& cppManager = *reinterpret_cast<IdCache*>(manager);
        return cppManager.getVersionId(spaceName, versionUniqueId);
    } catch (const std::exception& e) {
        return -1;
    }
}

int atv_id_cache_get_default_version_id(IdCacheManager* manager, const char* spaceName) {
    try {
        auto& cppManager = *reinterpret_cast<IdCache*>(manager);
        return cppManager.getDefaultVersionId(spaceName);
    } catch (const std::exception& e) {
        return -1;
    }
}

int atv_id_cache_get_vector_index_id(IdCacheManager* manager, const char* spaceName, int versionUniqueId) {
    try {
        auto& cppManager = *reinterpret_cast<IdCache*>(manager);
        return cppManager.getVectorIndexId(spaceName, versionUniqueId);
    } catch (const std::exception& e) {
        return -1;
    }
}

char* atv_id_cache_get_space_name_and_version_unique_id(IdCacheManager* manager, int versionId) {
    try {
        auto& cppManager = *reinterpret_cast<IdCache*>(manager);
        auto result = cppManager.getSpaceNameAndVersionUniqueId(versionId);
        nlohmann::json jsonResult = {{"spaceName", result.first}, {"versionUniqueId", result.second}};
        std::string jsonString = jsonResult.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const std::exception& e) {
        return nullptr;
    }
}

char* atv_id_cache_get_space_name_and_version_unique_id_by_vector_index_id(IdCacheManager* manager, int vectorIndexId) {
    try {
        auto& cppManager = *reinterpret_cast<IdCache*>(manager);
        auto result = cppManager.getSpaceNameAndVersionUniqueIdByVectorIndexId(vectorIndexId);
        nlohmann::json jsonResult = {{"spaceName", result.first}, {"versionUniqueId", result.second}};
        std::string jsonString = jsonResult.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const std::exception& e) {
        return nullptr;
    }
}

void atv_id_cache_clean(IdCacheManager* manager) {
    try {
        auto& cppManager = *reinterpret_cast<IdCache*>(manager);
        cppManager.clean();
    } catch (const std::exception& e) {
        spdlog::error("Error in atv_id_cache_clean: {}", e.what());
    }
}

void atv_id_cache_clear_space_name_cache(IdCacheManager* manager) {
    try {
        auto& cppManager = *reinterpret_cast<IdCache*>(manager);
        cppManager.clearSpaceNameCache();
    } catch (const std::exception& e) {
        spdlog::error("Error in atv_id_cache_clear_space_name_cache: {}", e.what());
    }
}
