#ifndef __ATINYVECTORS_DATABASE_MANAGER_HPP__
#define __ATINYVECTORS_DATABASE_MANAGER_HPP__

#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <mutex>
#include <string>
#include "Config.hpp"

namespace atinyvectors {

class DatabaseManager {
private:
    static std::unique_ptr<DatabaseManager> instance;
    static std::mutex instanceMutex;
    
    SQLite::Database db;
    std::string migrationPath;

    DatabaseManager(const std::string& dbFileName, const std::string& migrationDir);

    bool checkInfoTable();
    void updateDatabaseVersion(int newVersion, const std::string& projectVersion);

public:
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager& getInstance(
        const std::string& dbFileName = atinyvectors::Config::getInstance().getDbName(),
        const std::string& migrationDir = "db");

    SQLite::Database& getDatabase();

    void reset();
    void migrate();
    int getDatabaseVersion();

    void setMigrationPath(const std::string& path) { migrationPath = path; }
    const std::string& getMigrationPath() const { return migrationPath; }
};

}

#endif
