#include "algo/FaissIndexLRUCache.hpp"
#include "Snapshot.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "Config.hpp" 
#include "utils/Utils.hpp"
#include <SQLiteCpp/Backup.h>
#include <sqlite3.h>
#include <zip.h>
#include <filesystem>
#include <fstream>
#include <ctime>
#include <random>
#include <string>
#include <sstream>

using namespace atinyvectors::algo;
using namespace atinyvectors::utils;
namespace fs = std::filesystem;

namespace atinyvectors {

namespace {
// Internal helper functions
std::string getCurrentFormattedTime() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now); 

    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y%m%d%H%M"); 
    return oss.str();
}

void bindSnapshotParameters(SQLite::Statement& query, const Snapshot& snapshot) {
    query.bind(1, snapshot.requestJson);     // request_json
    query.bind(2, snapshot.fileName);        // file_name
    query.bind(3, snapshot.createdTimeUtc);  // created_time_utc
}

Snapshot createSnapshotFromQuery(SQLite::Statement& query) {
    return Snapshot(
        query.getColumn(0).getInt(),          // id
        query.getColumn(1).getString(),       // request_json
        query.getColumn(2).getString(),       // file_name
        query.getColumn(3).getInt64()         // created_time_utc
    );
}

std::vector<Snapshot> executeSelectQuery(SQLite::Statement& query) {
    std::vector<Snapshot> snapshots;
    while (query.executeStep()) {
        snapshots.push_back(createSnapshotFromQuery(query));
    }
    return snapshots;
}

std::vector<Snapshot> executeSelectQuery(const std::string& queryStr) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, queryStr);
    return executeSelectQuery(query);
}

void zipDirectoryAndDatabase(const std::string& directoryPath, const std::string& zipFileName, const std::string& dbBackupFileName) {
    int error = 0;
    zip_t* zip = zip_open(zipFileName.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!zip) {
        throw std::runtime_error("Failed to create ZIP file.");
    }

    // Add the database backup file to the ZIP archive at the root level
    zip_source_t* dbSource = zip_source_file(zip, dbBackupFileName.c_str(), 0, 0);
    if (dbSource) {
        if (zip_file_add(zip, fs::path(dbBackupFileName).filename().c_str(), dbSource, ZIP_FL_OVERWRITE) < 0) {
            zip_source_free(dbSource);
            spdlog::warn("Failed to add database backup file {} to ZIP archive", dbBackupFileName);
        }
    } else {
        spdlog::warn("Failed to add database backup file {} to ZIP archive", dbBackupFileName);
    }

    // Add all files from the specified directory to the ZIP archive
    for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            const std::string& filePath = entry.path().string();
            zip_source_t* source = zip_source_file(zip, filePath.c_str(), 0, 0);
            if (!source) {
                spdlog::warn("Failed to add file {} to ZIP archive", filePath);
                continue;
            }

            std::string relativePath = fs::relative(entry.path(), directoryPath).string();
            if (zip_file_add(zip, relativePath.c_str(), source, ZIP_FL_OVERWRITE) < 0) {
                zip_source_free(source);
                spdlog::warn("Failed to add file {} to ZIP archive", filePath);
            }
        }
    }

    zip_close(zip);
}

void backupDatabase(const std::string& backupFileName) {
    auto& db = DatabaseManager::getInstance().getDatabase();

    // Open a destination database file for backup
    SQLite::Database destDB(backupFileName, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    // Create a backup object
    SQLite::Backup backup(destDB, db);

    // Perform the backup
    int res = backup.executeStep();  // Backup all pages
    if (res != SQLITE_DONE) {
        spdlog::error("backupDatabase failed: backupFileName={} res={}", backupFileName, res);
        throw std::runtime_error("Failed to complete the backup.");
    }
}

void unzipToDirectory(const std::string& zipFileName, const std::string& destinationDirectory) {
    int error = 0;
    zip_t* zip = zip_open(zipFileName.c_str(), 0, &error);
    if (!zip) {
        throw std::runtime_error("Failed to open ZIP file.");
    }

    zip_int64_t numEntries = zip_get_num_entries(zip, 0);
    for (zip_uint64_t i = 0; i < numEntries; i++) {
        const char* fileName = zip_get_name(zip, i, 0);
        zip_file_t* file = zip_fopen_index(zip, i, 0);
        if (!file) {
            spdlog::warn("Failed to open file {} in ZIP archive", fileName);
            continue;
        }

        std::string outputPath = destinationDirectory + "/" + fileName;
        fs::create_directories(fs::path(outputPath).parent_path());
        std::ofstream outputFile(outputPath, std::ios::binary);
        if (!outputFile.is_open()) {
            zip_fclose(file);
            spdlog::warn("Failed to open output file {}", outputPath);
            continue;
        }

        char buffer[1024];
        zip_int64_t bytesRead;
        while ((bytesRead = zip_fread(file, buffer, sizeof(buffer))) > 0) {
            outputFile.write(buffer, bytesRead);
        }

        zip_fclose(file);
        outputFile.close();
    }

    zip_close(zip);
}

int generateRandomNumber(int min = 0, int max = 99999) {
    std::random_device rd; // Seed for random number engine
    std::mt19937 gen(rd()); // Mersenne Twister engine
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

std::string findBackupFile(const std::string& targetDirectory) {
    if (!fs::exists(targetDirectory) || !fs::is_directory(targetDirectory)) {
        throw std::runtime_error("Target directory does not exist or is not a directory.");
    }

    for (const auto& entry : fs::directory_iterator(targetDirectory)) {
        const auto& path = entry.path();

        // starts with "backup_" and ends with ".db"
        if (path.has_filename()) {
            const std::string filename = path.filename().string();
            if (filename.rfind("backup_", 0) == 0 && filename.size() > 3 && filename.substr(filename.size() - 3) == ".db") {
                return path.string();
            }
        }
    }

    throw std::runtime_error("No backup file found in the target directory.");
}

void createManifestJson(const std::string& snapshotDirectory, const std::vector<std::pair<std::string, int>>& indexes) {
    nlohmann::json manifest;
    manifest["version"] = Config::getInstance().getProjectVersion();
    manifest["create_date"] = getCurrentFormattedTime();

    for (const auto& index : indexes) {
        nlohmann::json indexInfo;
        indexInfo["spaceName"] = index.first;
        indexInfo["versionId"] = index.second;
        manifest["indexes"].push_back(indexInfo);
    }

    std::string manifestPath = snapshotDirectory + "/manifest.json";
    std::ofstream manifestFile(manifestPath);
    if (manifestFile.is_open()) {
        manifestFile << manifest.dump(4);
        manifestFile.close();
    } else {
        spdlog::error("Failed to create manifest.json at: {}", manifestPath);
    }
}

} // anonymous namespace

std::unique_ptr<SnapshotManager> SnapshotManager::instance;
std::mutex SnapshotManager::instanceMutex;

SnapshotManager::SnapshotManager() {
}

SnapshotManager& SnapshotManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new SnapshotManager());
    }
    return *instance;
}

int SnapshotManager::createSnapshot(const std::vector<std::pair<std::string, int>>& versionInfoList, const std::string& fileName, const std::string& metaDirectory) {
    cleanupStorage();

    auto& cache = IdCache::getInstance();

    // Ensure that the metaDirectory exists and normalize the path
    fs::path metaDirPath(metaDirectory);
    if (!fs::exists(metaDirPath)) {
        fs::create_directories(metaDirPath);
    }

    std::string dbBackupFileName = "backup_" + std::to_string(generateRandomNumber()) + ".db";
    backupDatabase(dbBackupFileName);

    nlohmann::json requestJson;

    for (const auto& [spaceName, versionId] : versionInfoList) {
        spdlog::info("Creating snapshot for spaceName: {}, versionId: {}", spaceName, versionId);

        int vectorIndexId = cache.getVectorIndexId(spaceName, versionId);
        auto index = FaissIndexLRUCache::getInstance().get(vectorIndexId);
        index->saveIndex();

        spdlog::debug("Index saved for spaceName: {}, versionId: {}", spaceName, versionId);
        requestJson["snapshots"].push_back({{"space_name", spaceName}, {"version_id", versionId}});
    }

    // Create manifest.json in the snapshot directory
    createManifestJson(metaDirectory, versionInfoList);

    zipDirectoryAndDatabase(metaDirectory, fileName, dbBackupFileName);

    std::remove(dbBackupFileName.c_str());

    long currentTime = getCurrentTimeUTC();

    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Transaction transaction(db);

    Snapshot snapshot;
    snapshot.requestJson = requestJson.dump();
    snapshot.fileName = fileName;
    snapshot.createdTimeUtc = currentTime;

    SQLite::Statement insertQuery(db, "INSERT INTO Snapshot (request_json, file_name, created_time_utc) VALUES (?, ?, ?)");
    bindSnapshotParameters(insertQuery, snapshot);
    insertQuery.exec();

    snapshot.id = static_cast<int>(db.getLastInsertRowid());
    transaction.commit();

    return snapshot.id;
}

void SnapshotManager::restoreSnapshot(const std::string& zipFileName, const std::string& targetDirectory) {
    // clean memory caches
    IdCache::getInstance().clean();
    FaissIndexLRUCache::getInstance().clean();

    // Create the target directory if it does not exist
    if (!fs::exists(targetDirectory)) {
        fs::create_directories(targetDirectory);
    }

    // Unzip the snapshot file to the target directory
    unzipToDirectory(zipFileName, targetDirectory);

    // Define the database file name and the temporary restore file name
    auto& dbManager = DatabaseManager::getInstance();
    std::string dbFileName = dbManager.getDatabase().getFilename();
    std::string tempRestoreFileName = findBackupFile(targetDirectory);

    try {
        // Open the restored backup database in read-only mode
        SQLite::Database backupDb(tempRestoreFileName, SQLite::OPEN_READONLY);

        // Check if the database is in-memory
        if (dbFileName == ":memory:") {
            // Use backup API to restore the in-memory database
            SQLite::Backup backup(dbManager.getDatabase(), backupDb);

            // Execute all steps at once
            int res = backup.executeStep();
            if (res != SQLITE_DONE) {
                throw std::runtime_error("Failed to restore in-memory database.");
            }
            spdlog::info("In-memory database restored successfully from file: {}", zipFileName);
        } else {
            // Use backup API to restore a file-based database
            SQLite::Backup backup(dbManager.getDatabase(), backupDb);

            // Execute all steps at once
            int res = backup.executeStep();
            if (res != SQLITE_DONE) {
                throw std::runtime_error("Failed to restore file-based database.");
            }
            spdlog::info("File-based database restored successfully from file: {}", zipFileName);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error occurred during database restoration: {}", e.what());
        throw;
    }
}

std::vector<Snapshot> SnapshotManager::getAllSnapshots() {
    return executeSelectQuery("SELECT id, request_json, file_name, created_time_utc FROM Snapshot");
}

Snapshot SnapshotManager::getSnapshotById(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, request_json, file_name, created_time_utc FROM Snapshot WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return createSnapshotFromQuery(query);
    }

    throw std::runtime_error("Snapshot not found");
}

void SnapshotManager::deleteSnapshot(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();

    SQLite::Transaction transaction(db);

    // Delete the snapshot from the table
    SQLite::Statement deleteQuery(db, "DELETE FROM Snapshot WHERE id = ?");
    deleteQuery.bind(1, id);
    deleteQuery.exec();

    transaction.commit();
}

void SnapshotManager::cleanupStorage() {
    std::string rootPath = Config::getInstance().getDataPath() + "/space/";

    if (!fs::exists(rootPath) || !fs::is_directory(rootPath)) {
        spdlog::warn("Root path '{}' does not exist or is not a directory.", rootPath);
        return;
    }

    for (const auto& entry : fs::directory_iterator(rootPath)) {
        if (entry.is_directory()) {
            std::string spaceName = entry.path().filename().string();
            spdlog::debug("Checking spaceName: {}", spaceName);

            try {
                bool exists = IdCache::getInstance().getSpaceExists(spaceName);

                if (!exists) {
                    fs::remove_all(entry.path());
                    spdlog::info("Deleted unused space directory: {}", entry.path().string());
                }
            }
            catch (const std::exception& e) {
                spdlog::error("Error while checking or deleting directory '{}': {}", entry.path().string(), e.what());
            }
        }
    }

    spdlog::info("Completed cleanup of storage directories.");
}

};
