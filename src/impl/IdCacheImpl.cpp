#include "IdCache.hpp"
#include "DatabaseManager.hpp"
#include "spdlog/spdlog.h"

namespace atinyvectors {

std::unique_ptr<IdCache> IdCache::instance = nullptr;
std::mutex IdCache::instanceMutex;

IdCache::IdCache() {
    spdlog::debug("IdCache constructor called.");
}

IdCache& IdCache::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        spdlog::debug("Creating a new instance of IdCache.");
        instance.reset(new IdCache());
    }
    return *instance;
}

int IdCache::getVersionId(const std::string& spaceName, int versionUniqueId) {
    if (versionUniqueId == 0) {
        versionUniqueId = getDefaultUniqueVersionId(spaceName);
    }

    auto key = std::make_pair(spaceName, versionUniqueId);
    auto it = forwardCache.find(key);

    if (it != forwardCache.end()) {
        return it->second;
    } else {
        std::lock_guard<std::mutex> lock(cacheMutex);
        int versionId = fetchFromDb(spaceName, versionUniqueId);

        forwardCache[key] = versionId;
        reverseCache[versionId] = key;

        return versionId;
    }
}

int IdCache::getDefaultVersionId(const std::string& spaceName) {
    return getVersionId(spaceName, getDefaultUniqueVersionId(spaceName));
}

int IdCache::getDefaultUniqueVersionId(const std::string& spaceName) {
    int outVersionUniqueId = 0;

    auto cacheIt = spaceNameCache.find(spaceName);
    if (cacheIt != spaceNameCache.end()) {
        outVersionUniqueId = cacheIt->second;
    } else {
        std::lock_guard<std::mutex> lock(spaceNameCacheMutex);

        auto& db = atinyvectors::DatabaseManager::getInstance().getDatabase();

        SQLite::Statement defaultVersionQuery(db,
            "SELECT v.unique_id FROM Space s "
            "JOIN Version v ON s.id = v.spaceId "
            "WHERE s.name = ? AND v.is_default = 1");
        defaultVersionQuery.bind(1, spaceName);

        if (defaultVersionQuery.executeStep()) {
            outVersionUniqueId = defaultVersionQuery.getColumn(0).getInt();
            spaceNameCache[spaceName] = outVersionUniqueId;
        } else {
            spdlog::error("No default version found for space: {}", spaceName);
            throw std::runtime_error("No default version found in the database.");
        }
    }

    return outVersionUniqueId;
}

int IdCache::getVectorIndexId(const std::string& spaceName, int versionUniqueId) {
    if (versionUniqueId == 0) {
        versionUniqueId = getDefaultUniqueVersionId(spaceName);
    }

    auto key = std::make_pair(spaceName, versionUniqueId);
    auto it = vectorIndexReverseCache.find(key);

    if (it != vectorIndexReverseCache.end()) {
        return it->second;
    } else {
        std::lock_guard<std::mutex> lock(vectorIndexCacheMutex);

        int versionId = getVersionId(spaceName, versionUniqueId);

        auto& db = DatabaseManager::getInstance().getDatabase();
        SQLite::Statement queryDefaultIndex(db, "SELECT id FROM VectorIndex WHERE versionId = ? AND is_default = 1");
        queryDefaultIndex.bind(1, versionId);

        if (queryDefaultIndex.executeStep()) {
            int vectorIndexId = queryDefaultIndex.getColumn(0).getInt();

            vectorIndexForwardCache[vectorIndexId] = key;
            vectorIndexReverseCache[key] = vectorIndexId;

            return vectorIndexId;
        } else {
            spdlog::error("No default vectorIndex found in the database for spaceName: {}, versionUniqueId: {}", spaceName, versionUniqueId);
            throw std::runtime_error("No default vectorIndex found in the database.");
        }
    }
}

std::pair<std::string, int> IdCache::getSpaceNameAndVersionUniqueId(int versionId) {
    auto it = reverseCache.find(versionId);

    if (it != reverseCache.end()) {
        return it->second;
    } else {
        return fetchByVersionIdFromDb(versionId);
    }
}

std::pair<std::string, int> IdCache::getSpaceNameAndVersionUniqueIdByVectorIndexId(int vectorIndexId) {
    auto it = vectorIndexForwardCache.find(vectorIndexId);
    if (it != vectorIndexForwardCache.end()) {
        return it->second;
    } else {
        std::lock_guard<std::mutex> lock(vectorIndexCacheMutex);

        auto& db = DatabaseManager::getInstance().getDatabase();
        SQLite::Statement query(db, "SELECT versionId FROM VectorIndex WHERE id = ?");
        query.bind(1, vectorIndexId);

        int versionId = -1;
        if (query.executeStep()) {
            versionId = query.getColumn(0).getInt();
        } else {
            spdlog::error("VectorIndex with id {} not found in the database.", vectorIndexId);
            throw std::runtime_error("VectorIndex not found in the database.");
        }

        std::pair<std::string, int> result = getSpaceNameAndVersionUniqueId(versionId);
        vectorIndexForwardCache[vectorIndexId] = result;
        vectorIndexReverseCache[result] = vectorIndexId;

        return result;
    }
}

void IdCache::clean() {
    std::lock_guard<std::mutex> lock1(cacheMutex);
    std::lock_guard<std::mutex> lock2(spaceNameCacheMutex);
    std::lock_guard<std::mutex> lock3(vectorIndexCacheMutex);
    std::lock_guard<std::mutex> lock4(spaceIdCacheMutex);

    spdlog::debug("Clearing all caches.");

    forwardCache.clear();
    reverseCache.clear();
    spaceNameCache.clear();
    vectorIndexCache.clear();
    vectorIndexForwardCache.clear();
    vectorIndexReverseCache.clear();
    rbacTokenCache.clear();
    sparseDataPoolByIndexIdCache.clear();
    spaceIdCache.clear();
}

void IdCache::clearSpaceNameCache() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    std::lock_guard<std::mutex> lock2(spaceIdCacheMutex);

    spdlog::debug("Clearing spaceName cache.");

    spaceNameCache.clear();
    spaceIdCache.clear();
}

int IdCache::fetchFromDb(const std::string& spaceName, int versionUniqueId) {
    auto& db = atinyvectors::DatabaseManager::getInstance().getDatabase();

    SQLite::Statement query(db, "SELECT V.id FROM Version V "
                                "JOIN Space S ON V.spaceId = S.id "
                                "WHERE S.name = ? AND V.unique_id = ?");
    query.bind(1, spaceName);
    query.bind(2, versionUniqueId);

    if (query.executeStep()) {
        int versionId = query.getColumn(0).getInt();

        forwardCache[std::make_pair(spaceName, versionUniqueId)] = versionId;
        reverseCache[versionId] = std::make_pair(spaceName, versionUniqueId);

        return versionId;
    } else {
        spdlog::error("No matching data found in the database for spaceName: {}, versionUniqueId: {}", spaceName, versionUniqueId);
        throw std::runtime_error("No matching data found in the database.");
    }
}

std::pair<std::string, int> IdCache::fetchByVersionIdFromDb(int versionId) {
    auto& db = atinyvectors::DatabaseManager::getInstance().getDatabase();

    SQLite::Statement query(db, "SELECT S.name, V.unique_id FROM Version V "
                                "JOIN Space S ON V.spaceId = S.id "
                                "WHERE V.id = ?");
    query.bind(1, versionId);

    if (query.executeStep()) {
        std::string spaceName = query.getColumn(0).getString();
        int versionUniqueId = query.getColumn(1).getInt();

        forwardCache[std::make_pair(spaceName, versionUniqueId)] = versionId;
        reverseCache[versionId] = std::make_pair(spaceName, versionUniqueId);

        return std::make_pair(spaceName, versionUniqueId);
    } else {
        spdlog::error("No matching data found in the database for versionId: {}", versionId);
        throw std::runtime_error("No matching data found in the database.");
    }
}

RbacToken IdCache::getRbacToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = rbacTokenCache.find(token);
    if (it != rbacTokenCache.end()) {
        RbacToken cachedToken = it->second;
        if (cachedToken.isExpire()) {
            spdlog::debug("Token expired, removing from cache: {}", token);
            rbacTokenCache.erase(it); // Remove expired token from cache
        } else {
            return cachedToken;
        }
    }

    RbacToken fetchedToken = fetchRbacTokenFromManager(token);
    rbacTokenCache[token] = fetchedToken;
    return fetchedToken;
}

RbacToken IdCache::fetchRbacTokenFromManager(const std::string& token) {
    try {
        RbacTokenManager& tokenManager = RbacTokenManager::getInstance();
        RbacToken fetchedToken = tokenManager.getTokenByToken(token);
        return fetchedToken;
    } catch (const std::runtime_error& e) {
        spdlog::error("Failed to fetch token from RbacTokenManager: {}", e.what());
        throw;
    }
}

SparseDataPool& IdCache::getSparseDataPool(int vectorIndexId) {
    auto it = sparseDataPoolByIndexIdCache.find(vectorIndexId);

    if (it != sparseDataPoolByIndexIdCache.end()) {
        return *(it->second);
    } else {
        auto pool = std::make_shared<SparseDataPool>();
        sparseDataPoolByIndexIdCache[vectorIndexId] = pool;

        return *pool;
    }
}

bool IdCache::getSpaceExists(const std::string& spaceName) {
    std::lock_guard<std::mutex> lock(spaceIdCacheMutex);
    bool exists = spaceIdCache.find(spaceName) != spaceIdCache.end();
    return exists;
}

int IdCache::getSpaceId(const std::string& spaceName) {
    {
        std::lock_guard<std::mutex> lock(spaceIdCacheMutex);
        auto it = spaceIdCache.find(spaceName);
        if (it != spaceIdCache.end()) {
            return it->second;
        }
    }

    // Fetch from DB
    int spaceId = fetchSpaceIdFromDb(spaceName);

    {
        std::lock_guard<std::mutex> lock(spaceIdCacheMutex);
        spaceIdCache[spaceName] = spaceId;
    }

    return spaceId;
}

int IdCache::fetchSpaceIdFromDb(const std::string& spaceName) {
    try {
        auto& db = DatabaseManager::getInstance().getDatabase();
        SQLite::Statement query(db, "SELECT id FROM Space WHERE name = ? LIMIT 1");
        query.bind(1, spaceName);

        if (query.executeStep()) {
            int spaceId = query.getColumn(0).getInt();
            spdlog::debug("Fetched spaceId from DB: {} = {}", spaceName, spaceId);
            return spaceId;
        } else {
            spdlog::debug("Space does not exist in DB: {}", spaceName);
            throw std::runtime_error("Space does not exist in the database.");
        }
    } catch (const std::exception& e) {
        spdlog::error("Error fetching spaceId for {}: {}", spaceName, e.what());
        throw;
    }
}

}  // namespace atinyvectors
