#include "DatabaseManager.hpp"
#include <SQLiteCpp/Backup.h>
#include <sqlite3.h>

using namespace atinyvectors;

namespace atinyvectors
{

std::unique_ptr<DatabaseManager> DatabaseManager::instance;
std::mutex DatabaseManager::instanceMutex;

DatabaseManager::DatabaseManager(const std::string& dbFileName)
    : db(dbFileName, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
}

DatabaseManager& DatabaseManager::getInstance(const std::string& dbFileName) {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new DatabaseManager(dbFileName));
    }
    return *instance;
}

SQLite::Database& DatabaseManager::getDatabase() {
    return db;
}

}
