#ifndef __ATINYVECTORS_RBACTOKEN_DTO_HPP__
#define __ATINYVECTORS_RBACTOKEN_DTO_HPP__

#include <iostream>
#include <string>
#include "nlohmann/json.hpp"

namespace atinyvectors {
namespace dto {

class RbacTokenDTOManager {
public:
    int getSpacePermission(const std::string& token);
    int getVersionPermission(const std::string& token);
    int getVectorPermission(const std::string& token);
    int getSnapshotPermission(const std::string& token);
    int getSystemPermission(const std::string& token);

    std::string generateJWTToken(int expireDays = 0);
    
    nlohmann::json listTokens();
    void deleteToken(const std::string& token);
    void updateToken(const std::string& token, const std::string& jsonStr);
    nlohmann::json newToken(const std::string& jsonStr, const std::string& token = "");
};

} // namespace dto
} // namespace atinyvectors

#endif // __ATINYVECTORS_RBACTOKEN_DTO_HPP__
