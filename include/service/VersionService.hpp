#ifndef __ATINYVECTORS_VERSION_SERVICE_HPP__
#define __ATINYVECTORS_VERSION_SERVICE_HPP__

#include <iostream>
#include <string>
#include "nlohmann/json.hpp"

using namespace std;

namespace atinyvectors
{
namespace service
{

class VersionServiceManager {
public:
    void createVersion(const std::string& spaceName, const std::string& jsonStr);
    nlohmann::json getByVersionId(const std::string& spaceName, int versionUniqueId);
    nlohmann::json getByVersionName(const std::string& spaceName, const std::string& versionName);
    nlohmann::json getLists(const std::string& spaceName, int start, int limit);
    nlohmann::json getDefaultVersion(const std::string& spaceName);
    void deleteByVersionId(const std::string& spaceName, int versionUniqueId);
};

}
}

#endif // __ATINYVECTORS_VERSION_SERVICE_HPP__
