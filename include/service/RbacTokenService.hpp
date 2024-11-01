#ifndef __ATINYVECTORS_RBACTOKEN_SERVICE_HPP__
#define __ATINYVECTORS_RBACTOKEN_SERVICE_HPP__

#include <iostream>
#include <string>
#include "nlohmann/json.hpp"

namespace atinyvectors {
namespace service {

class RbacTokenServiceManager {
public:
    int getSpacePermission(const std::string& token);
    int getVersionPermission(const std::string& token);
    int getVectorPermission(const std::string& token);
    int getSnapshotPermission(const std::string& token);
    int getSystemPermission(const std::string& token);
    int getSearchPermission(const std::string& token);
    int getSecurityPermission(const std::string& token);
    int getKeyvaluePermission(const std::string& token);

    std::string generateJWTToken(int expireDays = 0);
    
    nlohmann::json listTokens();
    void deleteToken(const std::string& token);
    void updateToken(const std::string& token, const std::string& jsonStr);
    nlohmann::json newToken(const std::string& jsonStr, const std::string& token = "");
};

} // namespace service
} // namespace atinyvectors

#endif // __ATINYVECTORS_RBACTOKEN_SERVICE_HPP__
