#ifndef __ATINYVECTORS_SNAPSHOT_SERVICE_HPP__
#define __ATINYVECTORS_SNAPSHOT_SERVICE_HPP__

#include <iostream>
#include <string>
#include "nlohmann/json.hpp"

using namespace std;

namespace atinyvectors
{
namespace service
{

class SnapshotServiceManager {
public:
    void createSnapshot(const std::string& jsonStr);
    void restoreSnapshot(const std::string& fileName);
    void deleteSnapshot(const std::string& filename);
    nlohmann::json listSnapshots();
    void deleteSnapshots();
};

}
}

#endif // __ATINYVECTORS_SNAPSHOT_SERVICE_HPP__
