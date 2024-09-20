#ifndef __ATINYVECTORS_RBACTOKEN_HPP__
#define __ATINYVECTORS_RBACTOKEN_HPP__

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <SQLiteCpp/SQLiteCpp.h>

namespace atinyvectors {

enum class Permission {
    Denied = 0,
    ReadOnly = 1,
    ReadWrite = 2
};

class RbacToken {
public:
    int id;
    std::string token;
    int user_id;
    Permission system_permission;
    Permission space_permission;
    Permission version_permission;
    Permission vector_permission;
    Permission snapshot_permission;
    long expire_time_utc;

    RbacToken() 
        : id(0), user_id(0), expire_time_utc(0), 
          system_permission(Permission::ReadOnly), space_permission(Permission::ReadOnly), 
          version_permission(Permission::ReadOnly), vector_permission(Permission::ReadOnly), 
          snapshot_permission(Permission::ReadOnly) {}

    RbacToken(int id, const std::string& token, int user_id, 
              Permission system_permission, Permission space_permission, 
              Permission version_permission, Permission vector_permission, 
              Permission snapshot_permission, long expire_time_utc)
        : id(id), token(token), user_id(user_id), 
          system_permission(system_permission), space_permission(space_permission), 
          version_permission(version_permission), vector_permission(vector_permission), 
          snapshot_permission(snapshot_permission), expire_time_utc(expire_time_utc) {}

    bool isExpire() const;
};

class RbacTokenManager {
private:
    static std::unique_ptr<RbacTokenManager> instance;
    static std::mutex instanceMutex;

    RbacTokenManager();

public:
    RbacTokenManager(const RbacTokenManager&) = delete;
    RbacTokenManager& operator=(const RbacTokenManager&) = delete;

    static RbacTokenManager& getInstance();
    
    void createTable();
    static std::string generateJWTToken(int expireDays = 0);

    int addToken(RbacToken& token, int expireDays);
    RbacToken newToken(int user_id, Permission system_permission, Permission space_permission,
                    Permission version_permission, Permission vector_permission,
                    Permission snapshot_permission, int expireDays, const std::string& token = "");
    std::vector<RbacToken> getAllTokens();
    RbacToken getTokenById(int id);
    RbacToken getTokenByToken(const std::string& token);
    void updateToken(const RbacToken& token);
    void deleteToken(int id);
    void deleteByToken(const std::string& token);
    void deleteAllExpireTokens();
};

} // namespace atinyvectors

#endif // __ATINYVECTORS_RBACTOKEN_HPP__
