#ifndef __ATINYVECTORS_SPACE_DTO_HPP__
#define __ATINYVECTORS_SPACE_DTO_HPP__

#include <iostream>
#include <string>
#include "nlohmann/json.hpp"
#include "ValueType.hpp"

using namespace std;

namespace atinyvectors
{
namespace dto
{

class SpaceDTOManager {
public:
    void createSpace(const std::string& jsonStr);
    void updateSpace(const std::string& spaceName, const std::string& jsonStr); // TODO: Generate updateSpace
    void deleteSpace(const std::string& spaceName, const std::string& jsonStr);
    
    nlohmann::json getBySpaceId(int spaceId);
    nlohmann::json getBySpaceName(const std::string& spaceName);
    nlohmann::json getLists();
};

}
}

#endif