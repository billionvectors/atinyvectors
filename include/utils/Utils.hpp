#ifndef __ATINYVECTORS_UTILS_HPP__
#define __ATINYVECTORS_UTILS_HPP__

#include <string>

namespace atinyvectors
{
namespace utils
{

long getCurrentTimeUTC();
std::string getDataPathByVersionUniqueId(const std::string& spaceName, int versionUniqueId);
std::string getIndexFilePath(const std::string& spaceName, int versionUniqueId, int vectorIndexId);

};
};

#endif