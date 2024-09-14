#include "Version.hpp"
#include "DatabaseManager.hpp"
#include <chrono>

namespace atinyvectors {

namespace {
// internal functions
void bindVersionParameters(SQLite::Statement& query, const Version& version) {
    query.bind(1, version.spaceId);            // spaceId
    query.bind(2, version.unique_id);          // unique_id
    query.bind(3, version.name);               // name
    query.bind(4, version.description);        // description
    query.bind(5, version.tag);                // tag
    query.bind(6, version.created_time_utc);   // created_time_utc
    query.bind(7, version.updated_time_utc);   // updated_time_utc
    query.bind(8, version.is_default ? 1 : 0); // is_default
}

Version createVersionFromQuery(SQLite::Statement& query) {
    return Version(
        query.getColumn(0).getInt(),        // id
        query.getColumn(1).getInt(),        // spaceId
        query.getColumn(2).getInt(),        // unique_id
        query.getColumn(3).getString(),     // name
        query.getColumn(4).getString(),     // description
        query.getColumn(5).getString(),     // tag
        query.getColumn(6).getInt64(),      // created_time_utc
        query.getColumn(7).getInt64(),      // updated_time_utc
        query.getColumn(8).getInt() == 1    // is_default (converted from int to bool)
    );
}

std::vector<Version> executeSelectQuery(SQLite::Statement& query) {
    std::vector<Version> versions;
    while (query.executeStep()) {
        versions.push_back(createVersionFromQuery(query));
    }
    return versions;
}

std::vector<Version> executeSelectQuery(const std::string& queryStr) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, queryStr);
    return executeSelectQuery(query);
}

long getCurrentTimeUTC() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

} // anonymous namespace

//
std::unique_ptr<VersionManager> VersionManager::instance;
std::mutex VersionManager::instanceMutex;

VersionManager::VersionManager() {
    createTable();
}

void VersionManager::createTable() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    db.exec("CREATE TABLE IF NOT EXISTS Version ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "spaceId INTEGER NOT NULL, "
            "unique_id INTEGER NOT NULL, "
            "name TEXT NOT NULL, "
            "description TEXT, "
            "tag TEXT, "
            "created_time_utc INTEGER, "
            "updated_time_utc INTEGER, "
            "is_default BOOLEAN DEFAULT 0, "
            "FOREIGN KEY(spaceId) REFERENCES Space(id) "
            ");");
}

VersionManager& VersionManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new VersionManager());
    }
    return *instance;
}

int VersionManager::addVersion(Version& version) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    
    // unique_id를 spaceId 별로 최대값 + 1으로 설정
    SQLite::Statement maxUniqueIdQuery(db, "SELECT IFNULL(MAX(unique_id), 0) + 1 FROM Version WHERE spaceId = ?");
    maxUniqueIdQuery.bind(1, version.spaceId);
    if (maxUniqueIdQuery.executeStep()) {
        version.unique_id = maxUniqueIdQuery.getColumn(0).getInt();
    } else {
        version.unique_id = 1; // 기본값
    }

    spdlog::debug("Calculated unique_id: {}", version.unique_id);

    // 해당 spaceId에 기본 버전이 존재하는지 확인
    SQLite::Statement checkDefaultQuery(db, "SELECT COUNT(*) FROM Version WHERE spaceId = ? AND is_default = 1");
    checkDefaultQuery.bind(1, version.spaceId);
    int defaultCount = 0;
    if (checkDefaultQuery.executeStep()) {
        defaultCount = checkDefaultQuery.getColumn(0).getInt();
    }

    if (defaultCount == 0) {
        // 기본 버전이 없으면, 새 버전을 기본 버전으로 설정
        version.is_default = true;
        spdlog::debug("No default version found for spaceId: {}. Setting is_default to true.", version.spaceId);
    } else if (version.is_default) {
        // 기본 버전이 이미 존재하고, 새 버전이 기본으로 설정되면 기존 기본 버전을 업데이트
        SQLite::Statement updateQuery(db, "UPDATE Version SET is_default = 0 WHERE spaceId = ?");
        updateQuery.bind(1, version.spaceId);
        updateQuery.exec();
        spdlog::debug("Updated existing default versions for spaceId: {} to is_default = false.", version.spaceId);
    }

    // 현재 시간을 UTC로 설정
    long currentTime = getCurrentTimeUTC();
    version.created_time_utc = currentTime;
    version.updated_time_utc = currentTime;

    spdlog::debug("Inserting version: spaceId={}, unique_id={}, name={}, description={}, tag={}, created_time_utc={}, updated_time_utc={}, is_default={}",
                 version.spaceId, version.unique_id, version.name, version.description, version.tag, version.created_time_utc, version.updated_time_utc, version.is_default);

    // 버전 삽입
    SQLite::Statement insertQuery(db, "INSERT INTO Version (spaceId, unique_id, name, description, tag, created_time_utc, updated_time_utc, is_default) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    bindVersionParameters(insertQuery, version);
    insertQuery.exec();

    version.id = static_cast<int>(db.getLastInsertRowid());
    spdlog::debug("Inserted new version with auto-assigned ID: {}", version.id);
    
    transaction.commit();

    return version.id;
}


std::vector<Version> VersionManager::getAllVersions() {
    return executeSelectQuery("SELECT id, spaceId, unique_id, name, description, tag, created_time_utc, updated_time_utc, is_default FROM Version");
}

Version VersionManager::getVersionById(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, spaceId, unique_id, name, description, tag, created_time_utc, updated_time_utc, is_default FROM Version WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return createVersionFromQuery(query);
    }

    throw std::runtime_error("Version not found");
}

Version VersionManager::getVersionByUniqueId(int spaceId, int unique_id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, spaceId, unique_id, name, description, tag, created_time_utc, updated_time_utc, is_default FROM Version WHERE spaceId = ? AND unique_id = ?");
    query.bind(1, spaceId);
    query.bind(2, unique_id);

    if (query.executeStep()) {
        return createVersionFromQuery(query);
    }

    throw std::runtime_error("Version not found with the specified spaceId and unique_id.");
}

std::vector<Version> VersionManager::getVersionsBySpaceId(int spaceId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, spaceId, unique_id, name, description, tag, created_time_utc, updated_time_utc, is_default FROM Version WHERE spaceId = ?");
    query.bind(1, spaceId);

    return executeSelectQuery(query);
}

Version VersionManager::getDefaultVersion(int spaceId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, spaceId, unique_id, name, description, tag, created_time_utc, updated_time_utc, is_default FROM Version WHERE spaceId = ? AND is_default = 1");
    query.bind(1, spaceId);

    if (query.executeStep()) {
        return createVersionFromQuery(query);
    }

    throw std::runtime_error("Default version not found for the specified space.");
}

void VersionManager::updateVersion(const Version& version) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    
    if (version.is_default) {
        SQLite::Statement updateQuery(db, "UPDATE Version SET is_default = 0 WHERE spaceId = ? AND id != ?");
        updateQuery.bind(1, version.spaceId);
        updateQuery.bind(2, version.id);
        updateQuery.exec();
    }

    long currentTime = getCurrentTimeUTC();

    Version updatedVersion = version;
    updatedVersion.updated_time_utc = currentTime;

    SQLite::Statement query(db, "UPDATE Version SET name = ?, description = ?, tag = ?, created_time_utc = ?, updated_time_utc = ?, is_default = ? WHERE id = ?");
    query.bind(1, updatedVersion.name);
    query.bind(2, updatedVersion.description);
    query.bind(3, updatedVersion.tag);
    query.bind(4, updatedVersion.created_time_utc);
    query.bind(5, updatedVersion.updated_time_utc);
    query.bind(6, updatedVersion.is_default ? 1 : 0);
    query.bind(7, updatedVersion.id);
    query.exec();
    
    transaction.commit();
}

void VersionManager::deleteVersion(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();

    SQLite::Transaction transaction(db);
    
    SQLite::Statement checkQuery(db, "SELECT spaceId, is_default FROM Version WHERE id = ?");
    checkQuery.bind(1, id);
    
    int spaceId = -1;
    bool isDefault = false;
    
    if (checkQuery.executeStep()) {
        spaceId = checkQuery.getColumn(0).getInt();
        isDefault = checkQuery.getColumn(1).getInt() == 1;
    }

    SQLite::Statement deleteQuery(db, "DELETE FROM Version WHERE id = ?");
    deleteQuery.bind(1, id);
    deleteQuery.exec();
    
    if (isDefault) {
        SQLite::Statement findRecentQuery(db, "SELECT id FROM Version WHERE spaceId = ? ORDER BY created_time_utc DESC LIMIT 1");
        findRecentQuery.bind(1, spaceId);
        
        if (findRecentQuery.executeStep()) {
            int recentId = findRecentQuery.getColumn(0).getInt();
            SQLite::Statement setDefaultQuery(db, "UPDATE Version SET is_default = 1 WHERE id = ?");
            setDefaultQuery.bind(1, recentId);
            setDefaultQuery.exec();
        }
    }

    transaction.commit();
}

};