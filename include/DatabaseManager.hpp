#ifndef __ATINYVECTORS_DATABASE_MANAGER_HPP__
#define __ATINYVECTORS_DATABASE_MANAGER_HPP__

#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <mutex>
#include <string>
#include "Config.hpp"

namespace atinyvectors
{

class DatabaseManager {
private:
    static std::unique_ptr<DatabaseManager> instance;
    static std::mutex instanceMutex;
    
    SQLite::Database db;

    DatabaseManager(const std::string& dbFileName);

public:
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager& getInstance(const std::string& dbFileName = atinyvectors::Config::getInstance().getDbName());

    SQLite::Database& getDatabase();
};

};

#endif