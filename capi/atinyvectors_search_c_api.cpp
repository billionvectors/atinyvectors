#include "atinyvectors_c_api.h"
#include "dto/SearchDTO.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

// C API for SearchDTOManager
SearchDTOManager* atv_search_dto_manager_new() {
    return reinterpret_cast<SearchDTOManager*>(new atinyvectors::dto::SearchDTOManager());
}

void atv_search_dto_manager_free(SearchDTOManager* manager) {
    delete reinterpret_cast<atinyvectors::dto::SearchDTOManager*>(manager);
}

char* atv_search_dto_search(SearchDTOManager* manager, const char* spaceName, const int versionUniqueId, const char* queryJsonStr, size_t k) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SearchDTOManager*>(manager);
        std::vector<std::pair<float, hnswlib::labeltype>> results = cppManager->search(spaceName, versionUniqueId, queryJsonStr, k);
        nlohmann::json jsonResult = cppManager->extractSearchResultsToJson(results);
        
        // Allocate memory and return JSON string
        std::string jsonString = jsonResult.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const nlohmann::json::exception& e) {
        return atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}
