#include "dto/SearchDTO.hpp"
#include "algo/HnswIndexLRUCache.hpp"
#include "Space.hpp"
#include "Version.hpp"
#include "VectorIndex.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "utils/Utils.hpp"

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using namespace atinyvectors::algo;
using namespace atinyvectors::utils;

namespace atinyvectors {
namespace dto {

// Function to perform a search using the query JSON string
std::vector<std::pair<float, hnswlib::labeltype>> SearchDTOManager::search(const std::string& spaceName, int versionUniqueId, const std::string& queryJsonStr, size_t k) {
    // Parse the JSON query string
    nlohmann::json queryJson = nlohmann::json::parse(queryJsonStr);

    // Extract the query vector from JSON
    std::vector<float> queryVector;
    if (queryJson.contains("vector") && queryJson["vector"].is_array()) {
        queryVector = queryJson["vector"].get<std::vector<float>>();
    } else {
        spdlog::error("Query JSON does not contain 'vector' field or it is not an array.");
        throw std::invalid_argument("Invalid query JSON format.");
    }

    // Find the correct vector index by space name and version unique ID
    int vectorIndexId = findVectorIndexBySpaceNameAndVersionUniqueId(spaceName, versionUniqueId);
    if (vectorIndexId == -1) {
        spdlog::error("Vector index not found for space: {} and versionUniqueId: {}", spaceName, versionUniqueId);
        throw std::runtime_error("Vector index not found.");
    }

    // Perform search using HnswIndexLRUCache
    auto hnswIndexManager = HnswIndexLRUCache::getInstance().get(vectorIndexId);
    return hnswIndexManager->search(queryVector, k);
}

// Function to find vector index by space name and version unique ID
int SearchDTOManager::findVectorIndexBySpaceNameAndVersionUniqueId(const std::string& spaceName, int& outVersionUniqueId) {
    IdCache& cache = IdCache::getInstance();

    if (outVersionUniqueId == 0) {
        outVersionUniqueId = cache.getDefaultUniqueVersionId(spaceName);
    }

    return IdCache::getInstance().getVectorIndexId(spaceName, outVersionUniqueId);
}

nlohmann::json SearchDTOManager::extractSearchResultsToJson(const std::vector<std::pair<float, hnswlib::labeltype>>& searchResults) {
    nlohmann::json result = nlohmann::json::array();
    for (const auto& [distance, label] : searchResults) {
        result.push_back({
            {"distance", distance},
            {"label", label}
        });
    }
    return result;
}

} // namespace dto
} // namespace atinyvectors
