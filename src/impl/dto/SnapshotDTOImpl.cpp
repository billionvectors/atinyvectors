#include "dto/SnapshotDTO.hpp"
#include "Snapshot.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Config.hpp"
#include "IdCache.hpp"
#include "utils/Utils.hpp"

#include <string>
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>

using namespace nlohmann;
using namespace atinyvectors::utils;

namespace fs = std::filesystem;

namespace atinyvectors
{
namespace dto
{
namespace
{

std::string getCurrentFormattedTime() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now); 

    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y%m%d%H%M"); 
    return oss.str();
}

std::string formatTimestampToReadableDate(const std::string& timestamp) {
    // Convert from yyyyMMddhhmm to yyyy-MM-dd hh:mm
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y%m%d%H%M");

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M"); // Format as yyyy-MM-dd hh:mm
    return oss.str();
}

std::string getSnapshotFileName() {
    std::string date = getCurrentFormattedTime();
    return "snapshot-" + date + ".zip";
}

std::string getSnapshotDirectory() {
    std::string fullPath = Config::getInstance().getDataPath() + "/snapshot/";

    try {
        if (!fs::exists(fullPath)) {
            fs::create_directories(fullPath);
            spdlog::debug("Created directory: {}", fullPath);
        }
    } catch (const fs::filesystem_error& e) {
        spdlog::error("Error creating directory {}: {}", fullPath, e.what());
    }
    
    return fullPath;
}

} // anonymous namespace

void SnapshotDTOManager::createSnapshot(const std::string& jsonStr) {
    json inputJson = json::parse(jsonStr);

    spdlog::info("Starting createSnapshot with inputJson: {}", inputJson.dump());

    try {
        std::vector<std::pair<std::string, int>> indexList;
        for (auto& [spaceName, versionData] : inputJson.items()) {
            spdlog::debug("inputJson: spaceName={} version={}", spaceName, versionData.dump());

            if (versionData.is_number()) {
                int versionUniqueId = versionData.get<int>();
                int versionId = IdCache::getInstance().getVersionId(spaceName, versionUniqueId);
                spdlog::debug("Found versionId: {} for spaceName: {}", versionId, spaceName);
                indexList.emplace_back(spaceName, versionUniqueId);
            } else if (versionData.is_array()) {
                for (const auto& versionIdJson : versionData) {
                    int versionUniqueId = versionIdJson.get<int>();
                    int versionId = IdCache::getInstance().getVersionId(spaceName, versionUniqueId);
                    spdlog::debug("Found versionId: {} for spaceName: {}", versionId, spaceName);
                    indexList.emplace_back(spaceName, versionUniqueId);
                }
            }
        }

        std::string snapshotFileName = getSnapshotFileName();
        std::string snapshotDirectory = getSnapshotDirectory();
        std::string snapshotFilePath = snapshotDirectory + snapshotFileName;

        fs::create_directories(snapshotDirectory);

        // Use Config's data path for metaDirectory
        std::string metaDirectory = Config::getInstance().getDataPath(); 

        // Add manifest.json and DB files to the zip file (backup whole DB)
        SnapshotManager::getInstance().createSnapshot(indexList, snapshotFilePath, metaDirectory);

        spdlog::info("Snapshot created successfully, file: {}", snapshotFilePath);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in createSnapshot. Error: {}", e.what());
        throw;
    }
}

void SnapshotDTOManager::restoreSnapshot(const std::string& fileName) {
    spdlog::info("Starting restoreSnapshot for fileName: {}", fileName);

    try {
        // Get the snapshot directory
        std::string snapshotDirectory = getSnapshotDirectory();
        std::string fullFilePath = snapshotDirectory + fileName; // Full path to the snapshot file

        // Check if the snapshot file exists
        if (!fs::exists(fullFilePath)) {
            spdlog::error("Snapshot file not found: {}", fullFilePath);
            throw std::runtime_error("Snapshot file not found.");
        }

        // Use the Config data path as the target directory for restoration
        std::string metaDirectory = Config::getInstance().getDataPath();

        // Restore the snapshot from the specified file
        SnapshotManager::getInstance().restoreSnapshot(fullFilePath, metaDirectory);
        spdlog::info("Snapshot restored successfully from file: {}", fileName);

    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in restoreSnapshot for fileName: {}. Error: {}", fileName, e.what());
        throw;
    }
}

nlohmann::json SnapshotDTOManager::listSnapshots() {
    spdlog::debug("Starting listSnapshots");

    try {
        nlohmann::json result;
        nlohmann::json snapshots = nlohmann::json::array();

        // List all snapshots in the directory
        std::string snapshotDirectory = getSnapshotDirectory();

        for (const auto& entry : fs::directory_iterator(snapshotDirectory)) {
            if (entry.is_regular_file()) {
                nlohmann::json snapshotJson;
                std::string fileName = entry.path().filename().string();
                
                // Extract versionUniqueId and date from file name
                if (fileName.rfind("snapshot-", 0) == 0 && fileName.size() > 18 && fileName.substr(fileName.size() - 4) == ".zip") {
                    auto pos1 = fileName.find("-");
                    auto pos2 = fileName.find("-", pos1 + 1);
                    auto pos3 = fileName.find("-", pos2 + 1);

                    if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
                        snapshotJson["version_id"] = std::stoi(fileName.substr(pos2 + 1, pos3 - pos2 - 1));
                        std::string timestamp = fileName.substr(pos3 + 1, fileName.find(".zip") - pos3 - 1);
                        snapshotJson["date"] = formatTimestampToReadableDate(timestamp);  // Format the date
                    }

                    snapshotJson["file_name"] = fileName;
                    snapshots.push_back(snapshotJson);
                }
            }
        }

        result["snapshots"] = snapshots;
        return result;

    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in listSnapshots. Error: {}", e.what());
        throw;
    }
}

void SnapshotDTOManager::deleteSnapshots() {
    spdlog::info("Starting deleteSnapshots");

    try {
        std::string snapshotDirectory = getSnapshotDirectory();

        // Delete all snapshot files in the directory
        for (const auto& entry : fs::directory_iterator(snapshotDirectory)) {
            if (entry.is_regular_file()) {
                fs::remove(entry.path());
                spdlog::info("Deleted snapshot file: {}", entry.path().string());
            }
        }

    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in deleteSnapshots. Error: {}", e.what());
        throw;
    }
}

}
}
