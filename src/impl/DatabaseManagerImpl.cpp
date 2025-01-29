#include "DatabaseManager.hpp"
#include <SQLiteCpp/Backup.h>
#include <sqlite3.h>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace atinyvectors;

namespace atinyvectors {

std::unique_ptr<DatabaseManager> DatabaseManager::instance;
std::mutex DatabaseManager::instanceMutex;

DatabaseManager::DatabaseManager(const std::string& dbFileName, const std::string& migrationDir)
    : db(dbFileName, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE), migrationPath(migrationDir) {
    migrate();   
}

DatabaseManager& DatabaseManager::getInstance(const std::string& dbFileName, const std::string& migrationDir) {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        if (dbFileName != ":memory:") {
            std::string fullDbPath = Config::getInstance().getDataPath() + "/" + dbFileName;
            instance.reset(new DatabaseManager(fullDbPath, migrationDir));
        } else {
            instance.reset(new DatabaseManager(dbFileName, migrationDir));
        }
    } else if (migrationDir != instance->migrationPath) {
        instance->setMigrationPath(migrationDir);
    }
    return *instance;
}

SQLite::Database& DatabaseManager::getDatabase() {
    return db;
}

void executeSqlFile(SQLite::Database& db, const std::string& filePath) {
    std::ifstream sqlFile(filePath);
    if (!sqlFile.is_open()) {
        throw std::runtime_error("Failed to open SQL file: " + filePath);
    }

    std::stringstream sqlBuffer;
    sqlBuffer << sqlFile.rdbuf();
    db.exec(sqlBuffer.str());
    spdlog::info("Executed SQL file: {}", filePath);
}

bool DatabaseManager::checkInfoTable() {
    try {
        db.exec("SELECT 1 FROM info LIMIT 1;");
        return true;
    } catch (const SQLite::Exception&) {
        return false;
    }
}

int DatabaseManager::getDatabaseVersion() {
    SQLite::Statement query(db, "SELECT dbversion FROM info LIMIT 1;");
    if (query.executeStep()) {
        return query.getColumn(0).getInt();
    }
    throw std::runtime_error("Failed to retrieve database version from info table.");
}

void DatabaseManager::updateDatabaseVersion(int newVersion, const std::string& projectVersion) {
    auto updatedTimeUtc = std::time(nullptr);
    SQLite::Statement query(db, 
        "UPDATE info SET dbversion = ?, version = ?, updated_time_utc = ?;");
    query.bind(1, newVersion);
    query.bind(2, projectVersion);
    query.bind(3, static_cast<long>(updatedTimeUtc));
    query.exec();
    spdlog::info("Updated database version to {}, project version: {}", newVersion, projectVersion);
}

void DatabaseManager::migrate() {
    spdlog::info("Starting database migration...");
    SQLite::Transaction transaction(db);
    try {
        int currentDbVersion = 0;
        bool resetRequired = false;

        // Check if info table exists
        if (!checkInfoTable()) {
            // Check if space table exists
            bool spaceTableExists = false;
            SQLite::Statement checkSpaceTableQuery(db, 
                "SELECT count(name) FROM sqlite_master WHERE type='table' AND name='Space';");
            if (checkSpaceTableQuery.executeStep()) {
                spaceTableExists = (checkSpaceTableQuery.getColumn(0).getInt() > 0);
            }

            if (!spaceTableExists) {
                spdlog::warn("Space table not found. Performing full reset.");
                executeSqlFile(db, migrationPath + "/reset.sql");
                resetRequired = true;
            } else {
                spdlog::warn("Info table not found but space table exists. Proceeding with migration.");
                currentDbVersion = 0;
            }
        } else {
            try {
                currentDbVersion = getDatabaseVersion();
            } catch (const std::exception&) {
                resetRequired = true;
                currentDbVersion = 0;
            }
        }

        // If reset was performed, skip migrations and only verify the latest version
        std::vector<std::pair<int, std::string>> migrationFiles;
        for (const auto& entry : std::filesystem::directory_iterator(migrationPath)) {
            std::string filePath = entry.path().string();
            if (filePath.find("migration_") != std::string::npos) {
                int migrationVersion = std::stoi(filePath.substr(filePath.find("migration_") + 10));
                migrationFiles.emplace_back(migrationVersion, filePath);
            }
        }

        // Sort migration files by version in ascending order
        std::sort(migrationFiles.begin(), migrationFiles.end());
        bool updateInfo = false;

        if (resetRequired) {
            if (!migrationFiles.empty()) {
                auto [latestVersion, latestFile] = migrationFiles.back();
                spdlog::info("Verifying the latest migration version: {}", latestFile);
                currentDbVersion = latestVersion;
                updateInfo = true;
            }
        } else {
            for (const auto& [version, filePath] : migrationFiles) {
                if (version > currentDbVersion) {
                    spdlog::info("Applying migration: {}", filePath);
                    executeSqlFile(db, filePath);
                    currentDbVersion = version;
                    updateInfo = true;
                }
            }
        }

        if (updateInfo) {
            // Update dbversion in info table
            std::string projectVersion = Config::getInstance().getProjectVersion(); // Use Config for project version
            spdlog::info("Updating dbversion={} and project version={} in info table.", currentDbVersion, projectVersion);
            updateDatabaseVersion(currentDbVersion, projectVersion);
        }

        transaction.commit();
        spdlog::info("Database migrated successfully to version {}", currentDbVersion);
    } catch (const std::exception& e) {
        spdlog::error("Database migration failed: {}", e.what());
        transaction.rollback(); // Rollback the transaction
        throw;
    }
}

void DatabaseManager::reset() {
    spdlog::info("Resetting database...");
    SQLite::Transaction transaction(db);
    try {
        executeSqlFile(db, migrationPath + "/reset.sql");

        // Find the latest dbversion from migration files
        std::vector<std::pair<int, std::string>> migrationFiles;
        for (const auto& entry : std::filesystem::directory_iterator(migrationPath)) {
            std::string filePath = entry.path().string();
            if (filePath.find("migration_") != std::string::npos) {
                int migrationVersion = std::stoi(filePath.substr(filePath.find("migration_") + 10));
                migrationFiles.emplace_back(migrationVersion, filePath);
            }
        }

        // Sort migration files by version in ascending order
        std::sort(migrationFiles.begin(), migrationFiles.end());

        int latestDbVersion = 0;
        if (!migrationFiles.empty()) {
            auto [latestVersion, latestFile] = migrationFiles.back();
            latestDbVersion = latestVersion;
            spdlog::info("Latest dbversion found: {}", latestDbVersion);
        } else {
            spdlog::warn("No migration files found. Setting dbversion to 0.");
        }

        // Insert version and dbversion into the info table
        const std::string projectVersion = Config::getInstance().getProjectVersion();
        spdlog::info("Updating info table with projectVersion={} and dbversion={}", projectVersion, latestDbVersion);

        SQLite::Statement insertInfoQuery(db, 
            "INSERT INTO info (version, dbversion, created_time_utc, updated_time_utc) VALUES (?, ?, strftime('%s', 'now'), strftime('%s', 'now'));");
        insertInfoQuery.bind(1, projectVersion);
        insertInfoQuery.bind(2, latestDbVersion);
        insertInfoQuery.exec();

        transaction.commit();
        spdlog::info("Database reset completed. Updated to version={} and dbversion={}", projectVersion, latestDbVersion);
    } catch (const std::exception& e) {
        spdlog::error("Database reset failed: {}", e.what());
        transaction.rollback();
        throw;
    }
}

}
