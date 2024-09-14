#include "dto/SearchDTO.hpp"
#include "Space.hpp"
#include "Version.hpp"
#include "VectorIndex.hpp"
#include "DatabaseManager.hpp"
#include "algo/HnswIndexLRUCache.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using namespace atinyvectors::algo;

namespace atinyvectors {
namespace dto {

// Function to perform a search using the query JSON string
std::vector<std::pair<float, hnswlib::labeltype>> SearchDTOManager::search(const std::string& spaceName, const std::string& queryJsonStr, size_t k) {
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

    // Find the correct vector index by space name
    int vectorIndexId = findVectorIndexBySpaceName(spaceName);
    if (vectorIndexId == -1) {
        spdlog::error("Vector index not found for space: {}", spaceName);
        throw std::runtime_error("Vector index not found.");
    }

    // Perform search using HnswIndexLRUCache
    std::string indexFileName = "index_file_" + std::to_string(vectorIndexId);
    auto hnswIndexManager = HnswIndexLRUCache::getInstance().get(vectorIndexId, indexFileName, 128, 10000);
    return hnswIndexManager->search(queryVector, k);
}

// Function to find vector index by space name
int SearchDTOManager::findVectorIndexBySpaceName(const std::string& spaceName) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT vi.id FROM Space s "
                                   "JOIN Version v ON s.id = v.spaceId "
                                   "JOIN VectorIndex vi ON vi.versionId = v.id "
                                   "WHERE s.name = ? AND vi.is_default = 1");
    query.bind(1, spaceName);

    if (query.executeStep()) {
        return query.getColumn(0).getInt();
    } else {
        spdlog::error("No default vector index found for space: {}", spaceName);
        return -1;
    }
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
