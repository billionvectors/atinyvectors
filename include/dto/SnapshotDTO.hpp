#ifndef __ATINYVECTORS_SNAPSHOT_DTO_HPP__
#define __ATINYVECTORS_SNAPSHOT_DTO_HPP__

#include <iostream>
#include <string>
#include "nlohmann/json.hpp"

using namespace std;

namespace atinyvectors
{
namespace dto
{

class SnapshotDTOManager {
public:
    void createSnapshot(const std::string& jsonStr);
    void restoreSnapshot(const std::string& fileName);
    nlohmann::json listSnapshots();
    void deleteSnapshots();
};

}
}

#endif
