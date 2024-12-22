#ifndef __ATINYVECTORS_RERANK_SERVICE_HPP__
#define __ATINYVECTORS_RERANK_SERVICE_HPP__

#include "service/SearchService.hpp"
#include "service/BM25Service.hpp"
#include "nlohmann/json.hpp"
#include <vector>
#include <string>

namespace atinyvectors {
namespace service {

class RerankServiceManager {
public:
    RerankServiceManager(SearchServiceManager& searchService, BM25ServiceManager& bm25Service);

    // Perform reranking using BM25 on initial search results
    nlohmann::json rerank(const std::string& spaceName, int versionUniqueId, const std::string& queryJsonStr, size_t topK);

    SearchServiceManager& getSearchServiceManager();
    BM25ServiceManager& getBM25ServiceManager();
    
private:
    SearchServiceManager& searchService;
    BM25ServiceManager& bm25Service;
};

} // namespace service
} // namespace atinyvectors

#endif // __ATINYVECTORS_RERANK_SERVICE_HPP__
