#ifndef __ATINYVECTORS_ID_CACHE_HPP__
#define __ATINYVECTORS_ID_CACHE_HPP__

#include <map>
#include <string>
#include <mutex>
#include <memory>
#include "RbacToken.hpp"

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

    RbacToken getRbacToken(const std::string& token);

    void clean();
    void clearSpaceNameCache();

private:
    std::mutex cacheMutex;
    std::mutex spaceNameCacheMutex;
    std::mutex vectorIndexCacheMutex;

    std::map<std::pair<std::string, int>, int> forwardCache;
    std::map<int, std::pair<std::string, int>> reverseCache;

    std::map<std::string, int> spaceNameCache;
    std::map<int, std::pair<std::string, int>> vectorIndexCache;

    std::map<int, std::pair<std::string, int>> vectorIndexForwardCache;
    std::map<std::pair<std::string, int>, int> vectorIndexReverseCache;

    std::map<std::string, RbacToken> rbacTokenCache;

    int fetchFromDb(const std::string& spaceName, int versionUniqueId);
    std::pair<std::string, int> fetchByVersionIdFromDb(int versionId);
    RbacToken fetchRbacTokenFromManager(const std::string& token);
};

};

#endif
