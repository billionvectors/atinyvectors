#include "algo/HnswIndexLRUCache.hpp"
#include "DatabaseManager.hpp"
#include "ValueType.hpp"
#include "spdlog/spdlog.h"

namespace atinyvectors
{
namespace algo
{

std::unique_ptr<HnswIndexLRUCache> HnswIndexLRUCache::instance;
std::mutex HnswIndexLRUCache::instanceMutex;

HnswIndexLRUCache& HnswIndexLRUCache::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new HnswIndexLRUCache());
    }

    return *instance;
}

std::shared_ptr<HnswIndexManager> HnswIndexLRUCache::get(int vectorIndexId, const std::string& indexFileName, int dim, int maxElements) {
    spdlog::info("Fetching HnswIndexManager for vectorIndexId: {}", vectorIndexId);
    auto it = cacheMap_.find(vectorIndexId);

    if (it == cacheMap_.end()) {
        spdlog::info("HnswIndexManager for vectorIndexId: {} not found in cache. Creating new one.", vectorIndexId);

        if (cacheList_.size() == capacity_) {
            int lruKey = cacheList_.back();
            cacheList_.pop_back();
            cacheMap_.erase(lruKey);
            spdlog::debug("Cache full. Removed least recently used entry for vectorIndexId: {}", lruKey);
        }

        auto& db = DatabaseManager::getInstance().getDatabase();
        
        SQLite::Statement query(db, "SELECT metricType FROM VectorIndexOptimizer WHERE vectorIndexId = ?");
        query.bind(1, vectorIndexId);

        MetricType metric = MetricType::L2;  // 기본값으로 L2 사용
        if (query.executeStep()) {
            int metricType = query.getColumn(0).getInt();
            
            // metricType에 따라 MetricType 설정
            switch (metricType) {
                case 0:
                    metric = MetricType::L2;
                    spdlog::info("Using L2 metric for vectorIndexId: {}", vectorIndexId);
                    break;
                case 1:
                    metric = MetricType::Cosine;
                    spdlog::info("Using Cosine metric for vectorIndexId: {}", vectorIndexId);
                    break;
                case 2:
                    metric = MetricType::InnerProduct;
                    spdlog::info("Using InnerProduct metric for vectorIndexId: {}", vectorIndexId);
                    break;
                default:
                    spdlog::error("Unknown metricType: {} for vectorIndexId: {}", metricType, vectorIndexId);
                    throw std::runtime_error("Unknown metricType");
            }
        } else {
            spdlog::error("Failed to fetch metricType for vectorIndexId: {}", vectorIndexId);
            throw std::runtime_error("Failed to fetch metricType");
        }

        auto manager = std::make_shared<HnswIndexManager>(indexFileName, dim, maxElements, metric);
        cacheList_.push_front(vectorIndexId);
        cacheMap_[vectorIndexId] = {manager, cacheList_.begin()};

        return manager;
    } else {
        spdlog::debug("HnswIndexManager for vectorIndexId: {} found in cache.", vectorIndexId);
        cacheList_.erase(it->second.second);
        cacheList_.push_front(vectorIndexId);
        it->second.second = cacheList_.begin();

        return it->second.first;
    }
}

// Function to return the current state of the cache for debugging purposes
std::string HnswIndexLRUCache::getCacheContents() const {
    std::string contents;
    for (const auto& key : cacheList_) {
        contents += std::to_string(key) + " ";
    }
    return contents;
}

};
};
