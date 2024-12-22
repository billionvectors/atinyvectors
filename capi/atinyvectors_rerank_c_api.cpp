#include "atinyvectors_c_api.h"
#include "service/RerankService.hpp"
#include "service/SearchService.hpp"
#include "service/BM25Service.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

// C API for RerankServiceManager
RerankServiceManager* atv_rerank_service_manager_new() {
    auto* searchService = new atinyvectors::service::SearchServiceManager();
    auto* bm25Service = new atinyvectors::service::BM25ServiceManager();
    return reinterpret_cast<RerankServiceManager*>(new atinyvectors::service::RerankServiceManager(*searchService, *bm25Service));
}

void atv_rerank_service_manager_free(RerankServiceManager* manager) {
    auto* cppManager = reinterpret_cast<atinyvectors::service::RerankServiceManager*>(manager);
    delete &(cppManager->getSearchServiceManager());
    delete &(cppManager->getBM25ServiceManager());
    delete cppManager;
}

char* atv_rerank_service_rerank(RerankServiceManager* manager, const char* spaceName, const int versionUniqueId, const char* queryJsonStr, size_t k) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::RerankServiceManager*>(manager);
        nlohmann::json rerankResults = cppManager->rerank(spaceName, versionUniqueId, queryJsonStr, k);

        // Allocate memory and return JSON string
        std::string jsonString = rerankResults.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const nlohmann::json::exception& e) {
        return atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}
