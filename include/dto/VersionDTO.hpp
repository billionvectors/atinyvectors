#ifndef __ATINYVECTORS_VERSION_DTO_HPP__
#define __ATINYVECTORS_VERSION_DTO_HPP__

#include <iostream>
#include <string>
#include "nlohmann/json.hpp"

using namespace std;

namespace atinyvectors
{
namespace dto
{

class VersionDTOManager {
public:
    void createVersion(const std::string& spaceName, const std::string& jsonStr);
    nlohmann::json getByVersionId(const std::string& spaceName, int versionUniqueId);
    nlohmann::json getByVersionName(const std::string& spaceName, const std::string& versionName);
    nlohmann::json getLists(const std::string& spaceName);
    nlohmann::json getDefaultVersion(const std::string& spaceName);
};

}
}

#endif
