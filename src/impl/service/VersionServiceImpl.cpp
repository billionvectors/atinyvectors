#include "service/VersionService.hpp"

#include <string>
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include "Version.hpp"
#include "Space.hpp"
#include "IdCache.hpp"

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using namespace nlohmann;

namespace atinyvectors
{
namespace service
{
namespace
{

nlohmann::json fetchVersionDetails(const Version& version) {
    nlohmann::json result;

    result["id"] = version.unique_id;
    result["name"] = version.name;
    result["description"] = version.description;
    result["tag"] = version.tag;
    result["created_time_utc"] = version.created_time_utc;
    result["updated_time_utc"] = version.updated_time_utc;
    result["is_default"] = version.is_default;

    return result;
}

} // anonymous namespace

void VersionServiceManager::createVersion(const std::string& spaceName, const std::string& jsonStr) {
    spdlog::info("Starting createVersion for spaceName: {}", spaceName);

    // Fetch the space using the space name
    try {
        auto space = SpaceManager::getInstance().getSpaceByName(spaceName);
        if (space.id <= 0) {
            spdlog::error("Space with name '{}' not found.", spaceName);
            throw std::runtime_error("Space not found.");
        }

        json parsedJson = json::parse(jsonStr);

        std::string versionName = "Default Version";
        std::string versionDescription = "Automatically created version";
        std::string versionTag = "v1";

        if (parsedJson.contains("name")) {
            versionName = parsedJson["name"];
        }
        if (parsedJson.contains("description")) {
            versionDescription = parsedJson["description"];
        }
        if (parsedJson.contains("tag")) {
            versionTag = parsedJson["tag"];
        }

        // Create and add Version
        Version version(0, space.id, 0, versionName, versionDescription, versionTag, 0, 0, true);
        int versionId = VersionManager::getInstance().addVersion(version);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in createVersion for spaceName: {}. Error: {}", spaceName, e.what());
        throw;
    }
}

nlohmann::json VersionServiceManager::getByVersionId(const std::string& spaceName, int versionUniqueId) {
    try {
        int versionId = IdCache::getInstance().getVersionId(spaceName, versionUniqueId);

        // Get version by spaceId and unique_id
        auto version = VersionManager::getInstance().getVersionById(versionId);

        return fetchVersionDetails(version);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getByVersionId for spaceName: {}, versionUniqueId: {}. Error: {}", spaceName, versionUniqueId, e.what());
        throw;
    }
}

nlohmann::json VersionServiceManager::getByVersionName(const std::string& spaceName, const std::string& versionName) {
    try {
        auto space = SpaceManager::getInstance().getSpaceByName(spaceName);
        if (space.id <= 0) {
            spdlog::error("Space with name '{}' not found.", spaceName);
            throw std::runtime_error("Space not found.");
        }

        auto versions = VersionManager::getInstance().getVersionsBySpaceId(space.id);
        for (const auto& version : versions) {
            if (version.name == versionName) {
                return fetchVersionDetails(version);
            }
        }

        spdlog::error("Version with name '{}' not found in space '{}'.", versionName, spaceName);
        throw std::runtime_error("Version not found.");
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getByVersionName for spaceName: {}, versionName: {}. Error: {}", spaceName, versionName, e.what());
        throw;
    }
}

nlohmann::json VersionServiceManager::getLists(const std::string& spaceName) {
    try {
        auto space = SpaceManager::getInstance().getSpaceByName(spaceName);
        if (space.id <= 0) {
            spdlog::error("Space with name '{}' not found.", spaceName);
            throw std::runtime_error("Space not found.");
        }

        nlohmann::json result;
        nlohmann::json values = nlohmann::json::array();

        auto versions = VersionManager::getInstance().getVersionsBySpaceId(space.id);

        for (const auto& version : versions) {
            nlohmann::json versionJson;
            versionJson["id"] = version.unique_id;
            versionJson["name"] = version.name;
            versionJson["description"] = version.description;
            versionJson["tag"] = version.tag;
            versionJson["created_time_utc"] = version.created_time_utc;
            versionJson["updated_time_utc"] = version.updated_time_utc;
            versionJson["is_default"] = version.is_default;
            values.push_back(versionJson);
        }

        result["values"] = values;

        return result;
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getLists for spaceName: {}. Error: {}", spaceName, e.what());
        throw;
    }
}

nlohmann::json VersionServiceManager::getDefaultVersion(const std::string& spaceName) {
    try {
        auto space = SpaceManager::getInstance().getSpaceByName(spaceName);
        if (space.id <= 0) {
            spdlog::error("Space with name '{}' not found.", spaceName);
            throw std::runtime_error("Space not found.");
        }

        Version defaultVersion = VersionManager::getInstance().getDefaultVersion(space.id);
        return fetchVersionDetails(defaultVersion);

    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getDefaultVersion for spaceName: {}. Error: {}", spaceName, e.what());
        throw;
    }
}

}
}
