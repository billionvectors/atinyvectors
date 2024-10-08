#ifndef __ATINYVECTORS_SEARCH_DTO_HPP__
#define __ATINYVECTORS_SEARCH_DTO_HPP__

#include <vector>
#include <string>
#include "nlohmann/json.hpp"
#include "hnswlib/hnswlib.h"

namespace atinyvectors {
namespace dto {

class SearchDTOManager {
public:
    // Method to perform a search based on a JSON query and return the top k results with version's Unique ID
    std::vector<std::pair<float, hnswlib::labeltype>> search(const std::string& spaceName, int versionUniqueId, const std::string& queryJsonStr, size_t k);

    // Extracts search results to JSON format
    nlohmann::json extractSearchResultsToJson(const std::vector<std::pair<float, hnswlib::labeltype>>& searchResults);

private:
    // Helper function to find the appropriate vector index by space name and version Unique ID
    int findVectorIndexBySpaceNameAndVersionUniqueId(const std::string& spaceName, int& outVersionUniqueId);
};

} // namespace dto
} // namespace atinyvectors

#endif // __ATINYVECTORS_SEARCH_DTO_HPP__
