#ifndef __ATINYVECTORS_ID_CACHE_HPP__
#define __ATINYVECTORS_ID_CACHE_HPP__

#include <map>
#include <string>
#include <mutex>
#include <memory>
#include "RbacToken.hpp"
#include "SparseDataPool.hpp"

namespace atinyvectors {

class IdCache {
private:
    static std::unique_ptr<IdCache> instance;
    static std::mutex instanceMutex;

    IdCache();

public:
    IdCache(const IdCache&) = delete;
    IdCache& operator=(const IdCache&) = delete;

    static IdCache& getInstance();

    int getVersionId(const std::string& spaceName, int versionUniqueId); 
    int getDefaultVersionId(const std::string& spaceName); 
    int getDefaultUniqueVersionId(const std::string& spaceName); 

    int getVectorIndexId(const std::string& spaceName, int versionUniqueId); 
    std::pair<std::string, int> getSpaceNameAndVersionUniqueId(int versionId);
    std::pair<std::string, int> getSpaceNameAndVersionUniqueIdByVectorIndexId(int vectorIndexId);

    bool getSpaceExists(const std::string& spaceName);
    int getSpaceId(const std::string& spaceName);
    RbacToken getRbacToken(const std::string& token);

    SparseDataPool& getSparseDataPool(int vectorIndexId);

    void clean();
    void clearSpaceNameCache();

private:
    std::mutex cacheMutex;
    std::mutex spaceNameCacheMutex;
    std::mutex vectorIndexCacheMutex;
    std::mutex spaceIdCacheMutex;

    std::map<std::pair<std::string, int>, int> forwardCache;
    std::map<int, std::pair<std::string, int>> reverseCache;

    std::map<std::string, int> spaceNameCache;
    std::map<int, std::pair<std::string, int>> vectorIndexCache;

    std::map<int, std::pair<std::string, int>> vectorIndexForwardCache;
    std::map<std::pair<std::string, int>, int> vectorIndexReverseCache;

    std::map<std::string, RbacToken> rbacTokenCache;
    
    std::map<std::string, int> spaceIdCache;

    std::map<int, std::shared_ptr<SparseDataPool>> sparseDataPoolByIndexIdCache;

    int fetchFromDb(const std::string& spaceName, int versionUniqueId);
    std::pair<std::string, int> fetchByVersionIdFromDb(int versionId);
    RbacToken fetchRbacTokenFromManager(const std::string& token);
    int fetchSpaceIdFromDb(const std::string& spaceName);
};

};

#endif
