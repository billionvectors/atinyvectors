#include "service/RerankService.hpp"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;

namespace atinyvectors {
namespace service {

RerankServiceManager::RerankServiceManager(SearchServiceManager& searchService, BM25ServiceManager& bm25Service)
    : searchService(searchService), bm25Service(bm25Service) {}

nlohmann::json RerankServiceManager::rerank(const std::string& spaceName, int versionUniqueId, const std::string& queryJsonStr, size_t topK) {
    try {
        spdlog::debug("Starting rerank process for space: {} with topK: {}", spaceName, topK);

        // Perform initial vector search
        auto initialResults = searchService.search(spaceName, versionUniqueId, queryJsonStr, topK);

        // Extract vector unique IDs from initial results
        std::vector<long> vectorUniqueIds;
        for (const auto& [distance, vectorUniqueId] : initialResults) {
            vectorUniqueIds.push_back(vectorUniqueId);
        }

        // Parse the query JSON to extract tokens for BM25
        json queryJson = json::parse(queryJsonStr);
        std::vector<std::string> queryTokens;
        if (queryJson.contains("tokens") && queryJson["tokens"].is_array()) {
            queryTokens = queryJson["tokens"].get<std::vector<std::string>>();
        } else {
            spdlog::warn("Query JSON does not contain 'tokens'. BM25 rerank will not proceed.");
            return searchService.extractSearchResultsToJson(initialResults);
        }

        // Perform BM25 rerank on the initial results
        auto bm25Results = bm25Service.searchWithVectorUniqueIdsToJson(spaceName, versionUniqueId, vectorUniqueIds, queryTokens);

        // Merge vector search distances with BM25 scores for final ranking
        std::unordered_map<long, double> bm25Scores;
        for (const auto& bm25Result : bm25Results) {
            long vectorUniqueId = bm25Result["vectorUniqueId"];
            double score = bm25Result["score"];
            bm25Scores[vectorUniqueId] = score;
        }

        // Combine scores and rerank
        std::vector<std::tuple<long, float, double>> combinedResults;
        for (const auto& [distance, vectorUniqueId] : initialResults) {
            double bm25Score = bm25Scores.count(vectorUniqueId) ? bm25Scores[vectorUniqueId] : 0.0;
            combinedResults.emplace_back(vectorUniqueId, distance, bm25Score);
        }

        // Sort combined results by BM25 score (desc), then by distance (asc)
        std::sort(combinedResults.begin(), combinedResults.end(), [](const auto& a, const auto& b) {
            if (std::get<2>(a) != std::get<2>(b)) {
                return std::get<2>(a) > std::get<2>(b); // BM25 score desc
            }
            return std::get<1>(a) < std::get<1>(b); // Distance asc
        });

        // Convert final results to JSON
        json finalResults = json::array();
        for (const auto& [vectorUniqueId, distance, bm25Score] : combinedResults) {
            finalResults.push_back({
                {"id", vectorUniqueId},
                {"distance", distance},
                {"bm25_score", bm25Score}
            });
        }

        spdlog::debug("Rerank process completed for space: {}", spaceName);
        return finalResults;
    } catch (const std::exception& e) {
        spdlog::error("Failed to perform rerank: {}", e.what());
        throw;
    }
}

SearchServiceManager& RerankServiceManager::getSearchServiceManager() {
    return searchService;
}

BM25ServiceManager& RerankServiceManager::getBM25ServiceManager() {
    return bm25Service;
}

} // namespace service
} // namespace atinyvectors
