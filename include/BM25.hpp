#ifndef __ATINYVECTORS_BM25_MANAGER_HPP__
#define __ATINYVECTORS_BM25_MANAGER_HPP__

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace atinyvectors {

class BM25Manager {
private:
    static std::unique_ptr<BM25Manager> instance;
    static std::mutex instanceMutex;

    BM25Manager();

public:
    BM25Manager(const BM25Manager&) = delete;
    BM25Manager& operator=(const BM25Manager&) = delete;

    static BM25Manager& getInstance();

    void addDocument(long vectorId, const std::string& doc, const std::vector<std::string>& tokens);
    std::vector<std::pair<long, double>> searchWithVectorIds(const std::vector<long>& vectorIds, const std::vector<std::string>& queryTokens);
    std::string getDocByVectorId(int vectorId);
};

} // namespace atinyvectors

#endif
