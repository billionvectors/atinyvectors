#include "algo/HnswIndexLRUCache.hpp"
#include "Space.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "Config.hpp"
#include "utils/Utils.hpp"

#include <filesystem>
#include <iostream>

using namespace atinyvectors::algo;
using namespace atinyvectors::utils;

namespace fs = std::filesystem;

namespace atinyvectors
{
namespace impl
{

// Internal helper functions
void bindSpaceParameters(SQLite::Statement& query, const Space& space) {
    query.bind(1, space.name);
    query.bind(2, space.description);
    query.bind(3, space.created_time_utc);
    query.bind(4, space.updated_time_utc);
}

Space createSpaceFromQuery(SQLite::Statement& query) {
    return Space(
        query.getColumn(0).getInt(),        // id
        query.getColumn(1).getString(),     // name
        query.getColumn(2).getString(),     // description
        query.getColumn(3).getInt64(),      // created_time_utc
        query.getColumn(4).getInt64()       // updated_time_utc
    );
}

std::vector<Space> executeSpaceSelectQuery(SQLite::Statement& query) {
    std::vector<Space> spaces;
    while (query.executeStep()) {
        spaces.push_back(createSpaceFromQuery(query));
    }
    return spaces;
}

std::vector<Space> executeSpaceSelectQuery(const std::string& queryStr) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, queryStr);
    return executeSpaceSelectQuery(query);
}

// Helper function to retrieve all version IDs associated with a space
std::vector<int> getVersionIdsBySpaceId(SQLite::Database& db, int spaceId) {
    std::vector<int> versionIds;
    SQLite::Statement query(db, "SELECT id FROM Version WHERE spaceId = ?");
    query.bind(1, spaceId);
    while (query.executeStep()) {
        versionIds.push_back(query.getColumn(0).getInt());
    }
    return versionIds;
}

// Helper function to retrieve all vector index IDs associated with a list of version IDs
std::vector<int> getVectorIndexIdsByVersionIds(SQLite::Database& db, const std::vector<int>& versionIds) {
    std::vector<int> vectorIndexIds;
    if (versionIds.empty()) return vectorIndexIds;

    std::string placeholders = "(";
    for (size_t i = 0; i < versionIds.size(); ++i) {
        placeholders += "?";
        if (i < versionIds.size() - 1) placeholders += ",";
    }
    placeholders += ")";

    std::string queryStr = "SELECT id FROM VectorIndex WHERE versionId IN " + placeholders;
    SQLite::Statement query(db, queryStr);
    for (size_t i = 0; i < versionIds.size(); ++i) {
        query.bind(static_cast<int>(i + 1), versionIds[i]);
    }

    while (query.executeStep()) {
        vectorIndexIds.push_back(query.getColumn(0).getInt());
    }
    return vectorIndexIds;
}

// Helper function to retrieve all vector IDs associated with a list of vector index IDs
std::vector<int> getVectorIdsByVectorIndexIds(SQLite::Database& db, const std::vector<int>& vectorIndexIds) {
    std::vector<int> vectorIds;
    if (vectorIndexIds.empty()) return vectorIds;

    std::string placeholders = "(";
    for (size_t i = 0; i < vectorIndexIds.size(); ++i) {
        placeholders += "?";
        if (i < vectorIndexIds.size() - 1) placeholders += ",";
    }
    placeholders += ")";

    std::string queryStr = "SELECT id FROM Vector WHERE versionId IN (SELECT versionId FROM VectorIndex WHERE id IN " + placeholders + ")";
    SQLite::Statement query(db, queryStr);
    for (size_t i = 0; i < vectorIndexIds.size(); ++i) {
        query.bind(static_cast<int>(i + 1), vectorIndexIds[i]);
    }

    while (query.executeStep()) {
        vectorIds.push_back(query.getColumn(0).getInt());
    }
    return vectorIds;
}

// Helper function to retrieve all vector IDs directly associated with vector indexes
std::vector<int> getVectorIdsByVectorIndexIdsAlternative(SQLite::Database& db, const std::vector<int>& vectorIndexIds) {
    std::vector<int> vectorIds;
    if (vectorIndexIds.empty()) return vectorIds;

    std::string placeholders = "(";
    for (size_t i = 0; i < vectorIndexIds.size(); ++i) {
        placeholders += "?";
        if (i < vectorIndexIds.size() - 1) placeholders += ",";
    }
    placeholders += ")";

    std::string queryStr = "SELECT id FROM Vector WHERE versionId IN (SELECT versionId FROM VectorIndex WHERE id IN " + placeholders + ")";
    SQLite::Statement query(db, queryStr);
    for (size_t i = 0; i < vectorIndexIds.size(); ++i) {
        query.bind(static_cast<int>(i + 1), vectorIndexIds[i]);
    }

    while (query.executeStep()) {
        vectorIds.push_back(query.getColumn(0).getInt());
    }
    return vectorIds;
}

};

using namespace atinyvectors::impl;

std::unique_ptr<SpaceManager> SpaceManager::instance;
std::mutex SpaceManager::instanceMutex;

SpaceManager::SpaceManager() {
    createTable();
}

void SpaceManager::createTable() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    db.exec("CREATE TABLE IF NOT EXISTS Space ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL, "
            "description TEXT, "
            "created_time_utc INTEGER, "
            "updated_time_utc INTEGER);");
            
    // Create an index on the name column
    db.exec("CREATE INDEX IF NOT EXISTS idx_space_name ON Space(name);");
}

SpaceManager& SpaceManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new SpaceManager());
    }
    return *instance;
}

int SpaceManager::addSpace(Space& space) {
    auto& db = DatabaseManager::getInstance().getDatabase();

    space.created_time_utc = getCurrentTimeUTC();
    space.updated_time_utc = getCurrentTimeUTC();

    SQLite::Statement query(db, "INSERT INTO Space (name, description, created_time_utc, updated_time_utc) VALUES (?, ?, ?, ?)");
    bindSpaceParameters(query, space);
    query.exec();

    int insertedId = static_cast<int>(db.getLastInsertRowid());
    space.id = insertedId;

    return insertedId;
}

std::vector<Space> SpaceManager::getAllSpaces() {
    return executeSpaceSelectQuery("SELECT id, name, description, created_time_utc, updated_time_utc FROM Space");
}

Space SpaceManager::getSpaceById(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, name, description, created_time_utc, updated_time_utc FROM Space WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return createSpaceFromQuery(query);
    }

    throw std::runtime_error("Space not found");
}

Space SpaceManager::getSpaceByName(const std::string& name) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, name, description, created_time_utc, updated_time_utc FROM Space WHERE name = ?");
    query.bind(1, name);

    if (query.executeStep()) {
        return createSpaceFromQuery(query);
    }

    throw std::runtime_error("Space with name '" + name + "' not found");
}

void SpaceManager::updateSpace(Space& space) {
    space.updated_time_utc = getCurrentTimeUTC();

    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "UPDATE Space SET name = ?, description = ?, created_time_utc = ?, updated_time_utc = ? WHERE id = ?");
    bindSpaceParameters(query, space);
    query.bind(5, space.id);
    query.exec();
}

void SpaceManager::deleteSpace(int spaceId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    auto space = getSpaceById(spaceId);
    auto spaceName = space.name;

    try {
        // Begin transaction
        db.exec("BEGIN TRANSACTION;");

        // 1. Retrieve all version IDs associated with the space
        std::vector<int> versionIds = getVersionIdsBySpaceId(db, spaceId);

        if (!versionIds.empty()) {
            // 2. Retrieve all vector index IDs associated with the versions
            std::vector<int> vectorIndexIds = getVectorIndexIdsByVersionIds(db, versionIds);

            if (!vectorIndexIds.empty()) {
                // 3. Retrieve all vector IDs associated with the vector indexes
                std::vector<int> vectorIds = getVectorIdsByVectorIndexIds(db, vectorIndexIds);

                if (!vectorIds.empty()) {
                    // 4. Delete from VectorMetadata where vectorId is in vectorIds
                    std::string placeholders = "(";
                    for (size_t i = 0; i < vectorIds.size(); ++i) {
                        placeholders += "?";
                        if (i < vectorIds.size() - 1) placeholders += ",";
                    }
                    placeholders += ")";

                    std::string deleteVectorMetadataQuery = "DELETE FROM VectorMetadata WHERE vectorId IN " + placeholders + ";";
                    SQLite::Statement stmtMetadata(db, deleteVectorMetadataQuery);
                    for (size_t i = 0; i < vectorIds.size(); ++i) {
                        stmtMetadata.bind(static_cast<int>(i + 1), vectorIds[i]);
                    }
                    stmtMetadata.exec();
                }

                // 5. Delete from VectorValue where vectorIndexId is in vectorIndexIds
                if (!vectorIndexIds.empty()) {
                    std::string placeholders = "(";
                    for (size_t i = 0; i < vectorIndexIds.size(); ++i) {
                        placeholders += "?";
                        if (i < vectorIndexIds.size() - 1) placeholders += ",";
                    }
                    placeholders += ")";

                    std::string deleteVectorValueQuery = "DELETE FROM VectorValue WHERE vectorIndexId IN " + placeholders + ";";
                    SQLite::Statement stmtVectorValue(db, deleteVectorValueQuery);
                    for (size_t i = 0; i < vectorIndexIds.size(); ++i) {
                        stmtVectorValue.bind(static_cast<int>(i + 1), vectorIndexIds[i]);
                    }
                    stmtVectorValue.exec();
                }

                // 6. Delete from Vector where id is in vectorIds
                if (!vectorIds.empty()) {
                    std::string placeholders = "(";
                    for (size_t i = 0; i < vectorIds.size(); ++i) {
                        placeholders += "?";
                        if (i < vectorIds.size() - 1) placeholders += ",";
                    }
                    placeholders += ")";

                    std::string deleteVectorQuery = "DELETE FROM Vector WHERE id IN " + placeholders + ";";
                    SQLite::Statement stmtVector(db, deleteVectorQuery);
                    for (size_t i = 0; i < vectorIds.size(); ++i) {
                        stmtVector.bind(static_cast<int>(i + 1), vectorIds[i]);
                    }
                    stmtVector.exec();
                }

                // 7. Delete from VectorIndex where id is in vectorIndexIds
                if (!vectorIndexIds.empty()) {
                    std::string placeholders = "(";
                    for (size_t i = 0; i < vectorIndexIds.size(); ++i) {
                        placeholders += "?";
                        if (i < vectorIndexIds.size() - 1) placeholders += ",";
                    }
                    placeholders += ")";

                    std::string deleteVectorIndexQuery = "DELETE FROM VectorIndex WHERE id IN " + placeholders + ";";
                    SQLite::Statement stmtVectorIndex(db, deleteVectorIndexQuery);
                    for (size_t i = 0; i < vectorIndexIds.size(); ++i) {
                        stmtVectorIndex.bind(static_cast<int>(i + 1), vectorIndexIds[i]);
                    }
                    stmtVectorIndex.exec();
                }
            }

            // 8. Delete from Version where id is in versionIds
            if (!versionIds.empty()) {
                std::string placeholders = "(";
                for (size_t i = 0; i < versionIds.size(); ++i) {
                    placeholders += "?";
                    if (i < versionIds.size() - 1) placeholders += ",";
                }
                placeholders += ")";

                std::string deleteVersionQuery = "DELETE FROM Version WHERE id IN " + placeholders + ";";
                SQLite::Statement stmtVersion(db, deleteVersionQuery);
                for (size_t i = 0; i < versionIds.size(); ++i) {
                    stmtVersion.bind(static_cast<int>(i + 1), versionIds[i]);
                }
                stmtVersion.exec();
            }
        }

        // 9. Finally, delete the Space
        SQLite::Statement deleteSpaceQuery(db, "DELETE FROM Space WHERE id = ?;");
        deleteSpaceQuery.bind(1, spaceId);
        deleteSpaceQuery.exec();

        // Commit transaction
        db.exec("COMMIT;");

        IdCache::getInstance().clean();
        HnswIndexLRUCache::getInstance().clean();
    }
    catch (const std::exception& e) {
        // Rollback transaction in case of error
        db.exec("ROLLBACK;");
        throw std::runtime_error(std::string("Failed to delete Space with id ") + std::to_string(spaceId) + ": " + e.what());
    }
}

};
