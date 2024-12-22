#ifndef __ATINYVECTORS_BM25_SERVICE_HPP__
#define __ATINYVECTORS_BM25_SERVICE_HPP__

#include <string>
#include <vector>
#include <unordered_map>
#include "nlohmann/json.hpp"

namespace atinyvectors {
namespace service {

class BM25ServiceManager {
public:
    // Method to add a document to the BM25 table
    void addDocument(const std::string& spaceName, int versionUniqueId, long vectorUniqueId, const std::string& doc, const std::vector<std::string>& tokens);

    // Method to perform a BM25 search with vector IDs
    nlohmann::json searchWithVectorUniqueIdsToJson(const std::string& spaceName, int versionUniqueId, const std::vector<long>& vectorUniqueIds, const std::vector<std::string>& queryTokens);

private:
    // Helper to convert search results to JSON
    nlohmann::json convertResultsToJson(const std::vector<std::pair<long, double>>& results);
};

} // namespace service
} // namespace atinyvectors

#endif // __ATINYVECTORS_BM25_SERVICE_HPP__
