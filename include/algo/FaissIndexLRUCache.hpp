#ifndef __ATINYVECTORS_FAISS_INDEX_LRU_CACHE_HPP__
#define __ATINYVECTORS_FAISS_INDEX_LRU_CACHE_HPP__

#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>
#include "algo/FaissIndexManager.hpp"
#include "Config.hpp"

namespace atinyvectors
{
namespace algo
{

class FaissIndexLRUCache {
public:
    FaissIndexLRUCache(size_t capacity = atinyvectors::Config::getInstance().getHnswIndexCacheCapacity()) : capacity_(capacity) {}
    
    std::shared_ptr<FaissIndexManager> get(int vectorIndexId);

    std::string getCacheContents() const;

    void clean();

    static FaissIndexLRUCache& getInstance();

private:
    FaissIndexLRUCache(const FaissIndexLRUCache&) = delete;
    FaissIndexLRUCache& operator=(const FaissIndexLRUCache&) = delete;

    static std::unique_ptr<FaissIndexLRUCache> instance;
    static std::mutex instanceMutex;

    size_t capacity_; // Maximum capacity of the cache
    std::list<int> cacheList_; // List of keys representing the cache order
    std::unordered_map<int, std::pair<std::shared_ptr<FaissIndexManager>, std::list<int>::iterator>> cacheMap_; // Map to store the cache entries
};

}; // namespace algo
}; // namespace atinyvectors

#endif // LRU_CACHE_H
