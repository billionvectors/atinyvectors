#include "Space.hpp"
#include "DatabaseManager.hpp"
#include "utils/Utils.hpp"

using namespace atinyvectors::utils;

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

void SpaceManager::deleteSpace(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "DELETE FROM Space WHERE id = ?");
    query.bind(1, id);
    query.exec();
}

};
