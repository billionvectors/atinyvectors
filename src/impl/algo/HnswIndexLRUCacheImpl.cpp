#include "algo/HnswIndexLRUCache.hpp"
#include "utils/Utils.hpp"
#include "DatabaseManager.hpp"
#include "ValueType.hpp"
#include "VectorIndex.hpp"
#include "Config.hpp"
#include "IdCache.hpp"

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;
using namespace atinyvectors;
using namespace atinyvectors::utils;

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

std::shared_ptr<HnswIndexManager> HnswIndexLRUCache::get(int vectorIndexId) {
    spdlog::debug("Fetching HnswIndexManager for vectorIndexId: {}", vectorIndexId);
    auto it = cacheMap_.find(vectorIndexId);

    if (it != cacheMap_.end()) {
        spdlog::debug("HnswIndexManager for vectorIndexId: {} found in cache.", vectorIndexId);
        // Update the LRU list
        cacheList_.erase(it->second.second);
        cacheList_.push_front(vectorIndexId);
        it->second.second = cacheList_.begin();
        return it->second.first;
    }

    spdlog::debug("HnswIndexManager for vectorIndexId: {} not found in cache. Creating new one.", vectorIndexId);

    // Check if cache is full
    if (cacheList_.size() == capacity_) {
        int lruKey = cacheList_.back();
        cacheList_.pop_back();
        cacheMap_.erase(lruKey);
        spdlog::debug("Cache full. Removed least recently used entry for vectorIndexId: {}", lruKey);
    }

    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT metricType, dimension, hnswConfigJson FROM VectorIndex WHERE id = ?");
    query.bind(1, vectorIndexId);

    MetricType metric = MetricType::L2;  // Default metric
    int dim = 0;
    std::string hnswConfigJson;

    if (query.executeStep()) {
        int metricType = query.getColumn(0).getInt();
        dim = query.getColumn(1).getInt();
        hnswConfigJson = query.getColumn(2).getString();

        metric = static_cast<MetricType>(metricType);
        spdlog::debug("Using {} metric for vectorIndexId: {}", metricType, vectorIndexId);

        if (metricType < 0 || metricType > 2) {
            spdlog::error("Unknown metricType: {} for vectorIndexId: {}", metricType, vectorIndexId);
            throw std::runtime_error("Unknown metricType");
        }
    } else {
        spdlog::error("VectorIndex with id {} not found.", vectorIndexId);
        throw std::runtime_error("VectorIndex not found");
    }

    // Parse HnswConfig JSON
    HnswConfig hnswConfig;
    if (!hnswConfigJson.empty()) {
        try {
            json parsedConfig = json::parse(hnswConfigJson);
            hnswConfig = HnswConfig::fromJson(parsedConfig);
        } catch (const std::exception& e) {
            spdlog::error("Failed to parse hnswConfigJson: {} for vectorIndexId: {}. Error: {}", hnswConfigJson, vectorIndexId, e.what());
            throw;
        }
    }

    // Create and cache the new HnswIndexManager
    int maxElements = Config::getInstance().getHnswMaxDataSize();

    auto spaceNameCache = IdCache::getInstance().getSpaceNameAndVersionUniqueIdByVectorIndexId(vectorIndexId);
    std::string indexFileName = getIndexFilePath(spaceNameCache.first, spaceNameCache.second, vectorIndexId);
    
    auto manager = std::make_shared<HnswIndexManager>(indexFileName, vectorIndexId, dim, maxElements, metric, hnswConfig);
    cacheList_.push_front(vectorIndexId);
    cacheMap_[vectorIndexId] = {manager, cacheList_.begin()};

    return manager;
}

// Function to clear the cache
void HnswIndexLRUCache::clean() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    cacheList_.clear();
    cacheMap_.clear();
    spdlog::debug("Cache has been cleaned.");
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
