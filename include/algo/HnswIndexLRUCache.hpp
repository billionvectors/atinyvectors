#ifndef __ATINYVECTORS_HNSW_INDEX_LRU_CACHE_HPP__
#define __ATINYVECTORS_HNSW_INDEX_LRU_CACHE_HPP__

#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>
#include "algo/HnswIndexManager.hpp"
#include "Config.hpp"

namespace atinyvectors
{
namespace algo
{

class HnswIndexLRUCache {
public:
    HnswIndexLRUCache(size_t capacity = atinyvectors::Config::getInstance().getHnswIndexCacheCapacity()) : capacity_(capacity) {}
    
    std::shared_ptr<HnswIndexManager> get(int vectorIndexId);

    std::string getCacheContents() const;

    void clean();

    static HnswIndexLRUCache& getInstance();

private:
    HnswIndexLRUCache(const HnswIndexLRUCache&) = delete;
    HnswIndexLRUCache& operator=(const HnswIndexLRUCache&) = delete;

    static std::unique_ptr<HnswIndexLRUCache> instance;
    static std::mutex instanceMutex;

    size_t capacity_; // Maximum capacity of the cache
    std::list<int> cacheList_; // List of keys representing the cache order
    std::unordered_map<int, std::pair<std::shared_ptr<HnswIndexManager>, std::list<int>::iterator>> cacheMap_; // Map to store the cache entries
};

}; // namespace algo
}; // namespace atinyvectors

#endif // LRU_CACHE_H
