#include "RbacToken.hpp"
#include "DatabaseManager.hpp"
#include "Config.hpp"
#include "utils/Utils.hpp"
#include <jwt-cpp/jwt.h>
#include <ctime>
#include <random>
#include <sstream>

using namespace atinyvectors::utils;

namespace atinyvectors {

std::unique_ptr<RbacTokenManager> RbacTokenManager::instance;
std::mutex RbacTokenManager::instanceMutex;

bool RbacToken::isExpire() const {
    long currentTime = getCurrentTimeUTC();
    return currentTime > expire_time_utc;
}

RbacTokenManager::RbacTokenManager() {
    createTable();
}

void RbacTokenManager::createTable() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    db.exec("CREATE TABLE IF NOT EXISTS RbacToken ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "token TEXT NOT NULL, "
            "user_id INTEGER NOT NULL, "
            "system_permission INTEGER NOT NULL, "
            "space_permission INTEGER NOT NULL, "
            "version_permission INTEGER NOT NULL, "
            "vector_permission INTEGER NOT NULL, "
            "snapshot_permission INTEGER NOT NULL, "
            "expire_time_utc INTEGER NOT NULL);");

    db.exec("CREATE INDEX IF NOT EXISTS idx_rbac_expire_time_utc ON RbacToken(expire_time_utc);");
    db.exec("CREATE INDEX IF NOT EXISTS idx_rbac_token_expire ON RbacToken(token, expire_time_utc);");
}

RbacTokenManager& RbacTokenManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new RbacTokenManager());
    }
    return *instance;
}

std::string RbacTokenManager::generateJWTToken(int expireDays) {
    using namespace std::chrono;

    auto& config = atinyvectors::Config::getInstance();

    if (expireDays == 0) {
        expireDays = config.getDefaultTokenExpireDays();
    }

    std::string jwt_key = config.getJwtTokenKey();

    auto now = system_clock::now();
    auto expire_time = now + hours(24 * expireDays);

    auto token = jwt::create()
                    .set_issued_at(now)
                    .set_expires_at(expire_time)
                    .sign(jwt::algorithm::hs256{jwt_key});

    return token;
}

int RbacTokenManager::addToken(RbacToken& token, int expireDays) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Transaction transaction(db);

    auto& config = atinyvectors::Config::getInstance();

    if (expireDays == 0) {
        expireDays = config.getDefaultTokenExpireDays();
    }
    
    long currentTime = getCurrentTimeUTC();
    token.expire_time_utc = currentTime + (expireDays * 24 * 60 * 60);
    
    // Only generate a new token if the current token is empty
    if (token.token.empty()) {
        token.token = generateJWTToken(expireDays);
    }

    SQLite::Statement insertQuery(db, 
        "INSERT INTO RbacToken (token, user_id, system_permission, space_permission, version_permission, vector_permission, snapshot_permission, expire_time_utc) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    
    insertQuery.bind(1, token.token);
    insertQuery.bind(2, token.user_id);
    insertQuery.bind(3, static_cast<int>(token.system_permission));
    insertQuery.bind(4, static_cast<int>(token.space_permission));
    insertQuery.bind(5, static_cast<int>(token.version_permission));
    insertQuery.bind(6, static_cast<int>(token.vector_permission));
    insertQuery.bind(7, static_cast<int>(token.snapshot_permission));
    insertQuery.bind(8, token.expire_time_utc);
    
    insertQuery.exec();
    token.id = static_cast<int>(db.getLastInsertRowid());
    transaction.commit();

    return token.id;
}

RbacToken RbacTokenManager::newToken(int user_id, Permission system_permission,
                                     Permission space_permission, Permission version_permission,
                                     Permission vector_permission, Permission snapshot_permission,
                                     int expireDays, const std::string& token) {
    RbacToken rbacToken;
    rbacToken.user_id = user_id;
    rbacToken.system_permission = system_permission;
    rbacToken.space_permission = space_permission;
    rbacToken.version_permission = version_permission;
    rbacToken.vector_permission = vector_permission;
    rbacToken.snapshot_permission = snapshot_permission;
    
    // Use the provided token or generate a new one if empty
    rbacToken.token = !token.empty() ? token : generateJWTToken(expireDays);

    addToken(rbacToken, expireDays);
    return rbacToken;
}

std::vector<RbacToken> RbacTokenManager::getAllTokens() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    long currentTime = getCurrentTimeUTC();

    // Query only non-expired tokens
    SQLite::Statement query(db, 
        "SELECT id, token, user_id, system_permission, space_permission, version_permission, vector_permission, snapshot_permission, expire_time_utc "
        "FROM RbacToken WHERE expire_time_utc > ?");

    query.bind(1, currentTime);

    std::vector<RbacToken> tokens;
    while (query.executeStep()) {
        tokens.emplace_back(
            query.getColumn(0).getInt(),
            query.getColumn(1).getString(),
            query.getColumn(2).getInt(),
            static_cast<Permission>(query.getColumn(3).getInt()),
            static_cast<Permission>(query.getColumn(4).getInt()),
            static_cast<Permission>(query.getColumn(5).getInt()),
            static_cast<Permission>(query.getColumn(6).getInt()),
            static_cast<Permission>(query.getColumn(7).getInt()),
            query.getColumn(8).getInt64()
        );
    }
    return tokens;
}


RbacToken RbacTokenManager::getTokenById(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, token, user_id, system_permission, space_permission, version_permission, vector_permission, snapshot_permission, expire_time_utc FROM RbacToken WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return RbacToken(
            query.getColumn(0).getInt(),
            query.getColumn(1).getString(),
            query.getColumn(2).getInt(),
            static_cast<Permission>(query.getColumn(3).getInt()),
            static_cast<Permission>(query.getColumn(4).getInt()),
            static_cast<Permission>(query.getColumn(5).getInt()),
            static_cast<Permission>(query.getColumn(6).getInt()),
            static_cast<Permission>(query.getColumn(7).getInt()),
            query.getColumn(8).getInt64()
        );
    }

    throw std::runtime_error("Token not found");
}

RbacToken RbacTokenManager::getTokenByToken(const std::string& token) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    long currentTime = getCurrentTimeUTC();

    // Query for the token that is not expired
    SQLite::Statement query(db, 
        "SELECT id, token, user_id, system_permission, space_permission, version_permission, vector_permission, snapshot_permission, expire_time_utc "
        "FROM RbacToken WHERE token = ? AND expire_time_utc > ?");

    query.bind(1, token);
    query.bind(2, currentTime);

    if (query.executeStep()) {
        // Log each fetched value before casting
        int id = query.getColumn(0).getInt();
        std::string fetchedToken = query.getColumn(1).getString();
        int userId = query.getColumn(2).getInt();
        int systemPermInt = query.getColumn(3).getInt();
        int spacePermInt = query.getColumn(4).getInt();
        int versionPermInt = query.getColumn(5).getInt();
        int vectorPermInt = query.getColumn(6).getInt();
        int snapshotPermInt = query.getColumn(7).getInt();
        long expireTime = query.getColumn(8).getInt64();

        // Create and return the RbacToken object
        RbacToken fetchedTokenObj = RbacToken(
            id,
            fetchedToken,
            userId,
            static_cast<Permission>(systemPermInt),
            static_cast<Permission>(spacePermInt),
            static_cast<Permission>(versionPermInt),
            static_cast<Permission>(vectorPermInt),
            static_cast<Permission>(snapshotPermInt),
            expireTime
        );

        return fetchedTokenObj;
    }

    throw std::runtime_error("Token not found or expired");
}

void RbacTokenManager::updateToken(const RbacToken& token) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Transaction transaction(db);

    SQLite::Statement query(db, 
        "UPDATE RbacToken SET token = ?, user_id = ?, system_permission = ?, space_permission = ?, version_permission = ?, vector_permission = ?, snapshot_permission = ?, expire_time_utc = ? WHERE id = ?");
    
    query.bind(1, token.token);
    query.bind(2, token.user_id);
    query.bind(3, static_cast<int>(token.system_permission));
    query.bind(4, static_cast<int>(token.space_permission));
    query.bind(5, static_cast<int>(token.version_permission));
    query.bind(6, static_cast<int>(token.vector_permission));
    query.bind(7, static_cast<int>(token.snapshot_permission));
    query.bind(8, token.expire_time_utc);
    query.bind(9, token.id);
    
    query.exec();
    transaction.commit();
}

void RbacTokenManager::deleteByToken(const std::string& token) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Transaction transaction(db);

    SQLite::Statement query(db, "DELETE FROM RbacToken WHERE token = ?");
    query.bind(1, token);
    query.exec();
    
    transaction.commit();
}

void RbacTokenManager::deleteAllExpireTokens() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Transaction transaction(db);

    long currentTime = getCurrentTimeUTC();
    SQLite::Statement query(db, "DELETE FROM RbacToken WHERE expire_time_utc < ?");
    query.bind(1, currentTime);
    query.exec();
    
    transaction.commit();
}

} // namespace atinyvectors