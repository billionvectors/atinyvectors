#include "dto/SearchDTO.hpp"
#include "algo/HnswIndexLRUCache.hpp"
#include "Space.hpp"
#include "Version.hpp"
#include "VectorIndex.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "SparseDataPool.hpp"
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
    nlohmann::json queryJson;
    try {
        queryJson = nlohmann::json::parse(queryJsonStr);
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("Failed to parse query JSON: {}", e.what());
        throw std::invalid_argument("Invalid JSON format.");
    }

    // Determine if the query is for Dense or Sparse Vector
    bool isSparse = false;
    if (queryJson.contains("vector") && queryJson["vector"].is_array()) {
        isSparse = false;
    }
    else if (queryJson.contains("sparse_data") && queryJson["sparse_data"].is_object()) {
        isSparse = true;
    }
    else {
        spdlog::error("Query JSON must contain either 'vector' for Dense or 'sparse_data' for Sparse Vector.");
        throw std::invalid_argument("Invalid query JSON format.");
    }

    // Find the correct vector index by space name and version unique ID
    int vectorIndexId = findVectorIndexBySpaceNameAndVersionUniqueId(spaceName, versionUniqueId);
    if (vectorIndexId == -1) {
        spdlog::error("Vector index not found for space: {} and versionUniqueId: {}", spaceName, versionUniqueId);
        throw std::runtime_error("Vector index not found.");
    }

    // Get the HnswIndexManager instance from the cache
    auto hnswIndexManager = HnswIndexLRUCache::getInstance().get(vectorIndexId);
    if (!hnswIndexManager) {
        spdlog::error("HnswIndexManager instance not found for vectorIndexId: {}", vectorIndexId);
        throw std::runtime_error("HnswIndexManager not found.");
    }

    // Perform search based on vector type
    if (isSparse) {
        // Extract Sparse Vector data
        nlohmann::json sparseData = queryJson["sparse_data"];
        if (!sparseData.contains("indices") || !sparseData.contains("values") || !sparseData["indices"].is_array() || !sparseData["values"].is_array()) {
            spdlog::error("Sparse data must contain 'indices' and 'values' arrays.");
            throw std::invalid_argument("Invalid sparse_data format.");
        }

        // Ensure 'indices' and 'values' arrays are of the same length
        if (sparseData["indices"].size() != sparseData["values"].size()) {
            spdlog::error("'indices' and 'values' arrays must be of the same length.");
            throw std::invalid_argument("'indices' and 'values' arrays length mismatch.");
        }

        // Construct the Sparse Vector
        SparseData sparseVector;
        for (size_t i = 0; i < sparseData["indices"].size(); ++i) {
            int index = sparseData["indices"][i].get<int>();
            float value = sparseData["values"][i].get<float>();
            sparseVector.emplace_back(index, value);
        }

        // Perform the search using the Sparse Vector
        return hnswIndexManager->search(&sparseVector, k);
    }
    else {
        // Extract Dense Vector data
        std::vector<float> queryVector;
        try {
            queryVector = queryJson["vector"].get<std::vector<float>>();
        } catch (const nlohmann::json::type_error& e) {
            spdlog::error("'vector' field must be an array of floats: {}", e.what());
            throw std::invalid_argument("Invalid 'vector' format.");
        }

        // Perform the search using the Dense Vector
        return hnswIndexManager->search(queryVector, k);
    }
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
