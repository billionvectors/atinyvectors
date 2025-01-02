#ifndef __ATINYVECTORS_VERSION_HPP__
#define __ATINYVECTORS_VERSION_HPP__

#include <string>
#include <vector>
#include <mutex>
#include <memory>

namespace atinyvectors
{

class Version {
public:
    int id;
    int unique_id;
    int spaceId;
    std::string name;
    std::string description;
    std::string tag;
    bool is_default;
    long created_time_utc;
    long updated_time_utc;
    
    Version() 
        : id(0), spaceId(0), unique_id(0), created_time_utc(0), updated_time_utc(0), is_default(false) {}

    Version(int id, int spaceId, int unique_id, const std::string& name, const std::string& description, const std::string& tag, long created_time_utc, long updated_time_utc, bool is_default = false)
        : id(id), spaceId(spaceId), unique_id(unique_id), name(name), description(description), tag(tag), created_time_utc(created_time_utc), updated_time_utc(updated_time_utc), is_default(is_default) {}
};

class VersionManager {
private:
    static std::unique_ptr<VersionManager> instance;
    static std::mutex instanceMutex;

    VersionManager();

public:
    VersionManager(const VersionManager&) = delete;
    VersionManager& operator=(const VersionManager&) = delete;

    static VersionManager& getInstance();

    int addVersion(Version& version);
    std::vector<Version> getAllVersions();
    Version getVersionById(int id);
    Version getVersionByUniqueId(int spaceId, int unique_id);
    std::vector<Version> getVersionsBySpaceId(int spaceId, int start, int limit);
    Version getDefaultVersion(int spaceId);
    void updateVersion(const Version& version);
    void deleteVersion(int id);
    int getTotalCountBySpaceId(int spaceId);
};

};

#endif
