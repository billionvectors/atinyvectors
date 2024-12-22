#include "service/BM25Service.hpp"
#include "BM25.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;

namespace atinyvectors {
namespace service {

void BM25ServiceManager::addDocument(const std::string& spaceName, int versionUniqueId, long vectorUniqueId, const std::string& doc, const std::vector<std::string>& tokens) {
    try {
        // Get versionId using IdCache
        int versionId = IdCache::getInstance().getVersionId(spaceName, versionUniqueId);

        // Fetch vectorId using vectorUniqueId and versionId
        auto& db = DatabaseManager::getInstance().getDatabase();
        SQLite::Statement query(db, "SELECT id FROM Vector WHERE unique_id = ? AND versionId = ? AND deleted = 0");
        query.bind(1, vectorUniqueId);
        query.bind(2, versionId);

        if (!query.executeStep()) {
            spdlog::error("Vector with unique_id={} and versionId={} not found in spaceName={}", vectorUniqueId, versionId, spaceName);
            throw std::runtime_error("Vector not found.");
        }

        long vectorId = query.getColumn(0).getInt64();

        spdlog::debug("Adding document: spaceName={}, versionUniqueId={}, vectorUniqueId={}, vectorId={}, doc={}", spaceName, versionUniqueId, vectorUniqueId, vectorId, doc);

        // Add document to BM25Manager
        BM25Manager::getInstance().addDocument(vectorId, doc, tokens);

        spdlog::debug("Document added successfully: spaceName={}, versionUniqueId={}, vectorUniqueId={}, vectorId={}", spaceName, versionUniqueId, vectorUniqueId, vectorId);
    } catch (const std::exception& e) {
        spdlog::error("Failed to add document: {}", e.what());
        throw;
    }
}

nlohmann::json BM25ServiceManager::searchWithVectorUniqueIdsToJson(const std::string& spaceName, int versionUniqueId, const std::vector<long>& vectorUniqueIds, const std::vector<std::string>& queryTokens) {
    try {
        // Get versionId using IdCache
        int versionId = IdCache::getInstance().getVersionId(spaceName, versionUniqueId);

        spdlog::debug("Performing BM25 search in spaceName={} with vectorUniqueIds. Query tokens: {}", spaceName, queryTokens.size());

        // Optimize to fetch vectorIds in a single query
        auto& db = DatabaseManager::getInstance().getDatabase();

        if (vectorUniqueIds.empty()) {
            spdlog::warn("No vectorUniqueIds provided for BM25 search in spaceName={}", spaceName);
            return nlohmann::json::array();
        }

        // Build the SQL query dynamically for IN clause
        std::ostringstream queryStream;
        queryStream << "SELECT id FROM Vector WHERE versionId = ? AND deleted = 0 AND unique_id IN (";
        for (size_t i = 0; i < vectorUniqueIds.size(); ++i) {
            queryStream << "?";
            if (i < vectorUniqueIds.size() - 1) {
                queryStream << ", ";
            }
        }
        queryStream << ")";

        SQLite::Statement query(db, queryStream.str());

        // Bind parameters: versionId and unique_id values
        query.bind(1, versionId);
        for (size_t i = 0; i < vectorUniqueIds.size(); ++i) {
            query.bind(static_cast<int>(i + 2), vectorUniqueIds[i]);
        }

        // Collect vectorIds from the query result
        std::vector<long> vectorIds;
        while (query.executeStep()) {
            vectorIds.push_back(query.getColumn(0).getInt64());
        }

        if (vectorIds.empty()) {
            spdlog::warn("No matching vectorIds found for vectorUniqueIds in spaceName={}", spaceName);
        }

        // Perform search using vectorIds
        auto results = BM25Manager::getInstance().searchWithVectorIds(vectorIds, queryTokens);

        spdlog::debug("BM25 search completed in spaceName={}. Results count: {}", spaceName, results.size());
        return convertResultsToJson(results);
    } catch (const std::exception& e) {
        spdlog::error("BM25 search failed in spaceName={}: {}", spaceName, e.what());
        throw;
    }
}

nlohmann::json BM25ServiceManager::convertResultsToJson(const std::vector<std::pair<long, double>>& results) {
    // Collect all vectorIds from the results
    std::vector<long> vectorIds;
    for (const auto& [vectorId, score] : results) {
        vectorIds.push_back(vectorId);
    }

    // If no results, return an empty JSON array
    if (vectorIds.empty()) {
        return nlohmann::json::array();
    }

    // Prepare IN query for vectorIds
    std::ostringstream idListStream;
    idListStream << "(";
    for (size_t i = 0; i < vectorIds.size(); ++i) {
        idListStream << vectorIds[i];
        if (i < vectorIds.size() - 1) {
            idListStream << ", ";
        }
    }
    idListStream << ")";
    std::string idList = idListStream.str();

    // Query all vectorUniqueIds at once
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, unique_id FROM Vector WHERE id IN " + idList + " AND deleted = 0");

    // Build a map of vectorId to vectorUniqueId
    std::unordered_map<long, long> idToUniqueIdMap;
    while (query.executeStep()) {
        long vectorId = query.getColumn(0).getInt64();
        long vectorUniqueId = query.getColumn(1).getInt64();
        idToUniqueIdMap[vectorId] = vectorUniqueId;
    }

    // Convert results to JSON using the map
    nlohmann::json resultJson = nlohmann::json::array();
    for (const auto& [vectorId, score] : results) {
        if (idToUniqueIdMap.find(vectorId) != idToUniqueIdMap.end()) {
            resultJson.push_back({
                {"vectorUniqueId", idToUniqueIdMap[vectorId]},
                {"score", score}
            });
        } else {
            spdlog::warn("Vector with id={} not found in Vector table during result conversion.", vectorId);
        }
    }

    return resultJson;
}

} // namespace service
} // namespace atinyvectors
