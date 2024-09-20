#include "utils/Utils.hpp"
#include "Config.hpp"

#include <chrono>
#include <string>
#include <filesystem> 
#include <spdlog/spdlog.h>

namespace atinyvectors
{
namespace utils
{

long getCurrentTimeUTC() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string getDataPathByVersionUniqueId(const std::string& spaceName, int versionUniqueId) {
    std::string basePath = Config::getInstance().getDataPath();

    std::string fullPath = basePath + spaceName + "/" + std::to_string(versionUniqueId);

    try {
        if (!std::filesystem::exists(fullPath)) {
            std::filesystem::create_directories(fullPath);
            spdlog::debug("Created directory: {}", fullPath);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Error creating directory {}: {}", fullPath, e.what());
    }

    return fullPath;
}

std::string getIndexFilePath(const std::string& spaceName, int versionUniqueId, int vectorIndexId) {
    std::string fullPath = getDataPathByVersionUniqueId(spaceName, versionUniqueId) + "/index/";

    try {
        if (!std::filesystem::exists(fullPath)) {
            std::filesystem::create_directories(fullPath);
            spdlog::debug("Created directory: {}", fullPath);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Error creating directory {}: {}", fullPath, e.what());
    }
    
    return fullPath + "index_file_" + std::to_string(vectorIndexId) + ".idx";
}

};
};
