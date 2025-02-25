#include "service/SpaceService.hpp"
#include "utils/Utils.hpp"

#include "Space.hpp"
#include "Version.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Config.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"

#include <string>
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include <regex>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using namespace nlohmann;
using namespace atinyvectors::utils;

namespace atinyvectors
{
namespace service
{
namespace
{

MetricType metricTypeFromString(const std::string& metric);

void processDenseConfiguration(const json& parsedJson, int versionId);
void processSparseConfiguration(const json& parsedJson, int versionId);
void processIndexesConfiguration(const json& parsedJson, int versionId);

int createDenseVectorIndex(int versionId, const std::string& indexName, int denseDimension, const std::string& denseMetric,
                      const HnswConfig& hnswConfig, const QuantizationConfig& quantizationConfig, bool is_default);

int createSparseVectorIndex(int versionId, const std::string& indexName, const std::string& sparseMetric, bool is_default);

MetricType metricTypeFromString(const std::string& metric) {
    std::string metricLower = metric;
    std::transform(metricLower.begin(), metricLower.end(), metricLower.begin(), ::tolower);

    if (metricLower == "l2") return MetricType::L2;
    if (metricLower == "cosine") return MetricType::Cosine;
    if (metricLower == "inner_product") return MetricType::InnerProduct;
    throw std::invalid_argument("Unknown metric type: " + metric);
}

void processDenseConfiguration(const json& parsedJson, int versionId) {
    int denseDimension = parsedJson.value("dimension", 0);
    std::string denseMetric = parsedJson.value("metric", "l2");
    std::transform(denseMetric.begin(), denseMetric.end(), denseMetric.begin(), ::tolower);

    HnswConfig defaultHnswConfig;
    if (parsedJson.contains("hnsw_config")) {
        json hnswConfigJson = parsedJson["hnsw_config"];
        
        if (hnswConfigJson.contains("M")) {
            defaultHnswConfig.M = hnswConfigJson["M"];
        } else if (hnswConfigJson.contains("m")) {
            defaultHnswConfig.M = hnswConfigJson["m"];
        } else {
            defaultHnswConfig.M = Config::getInstance().getM();
        }

        defaultHnswConfig.EfConstruct = hnswConfigJson.value("ef_construct", Config::getInstance().getEfConstruction());
    }

    QuantizationConfig defaultQuantizationConfig;
    if (parsedJson.contains("quantization_config")) {
        json quantizationJson = parsedJson["quantization_config"];
        defaultQuantizationConfig = QuantizationConfig::fromJson(quantizationJson);
    }

    const std::string& defaultDenseIndexName = Config::getInstance().getDefaultDenseIndexName();

    if (parsedJson.contains(defaultDenseIndexName)) {
        const json& denseJson = parsedJson[defaultDenseIndexName];

        denseDimension = denseJson.value("dimension", denseDimension);
        denseMetric = denseJson.value("metric", denseMetric);
        std::transform(denseMetric.begin(), denseMetric.end(), denseMetric.begin(), ::tolower);

        HnswConfig hnswConfig = defaultHnswConfig;
        if (denseJson.contains("hnsw_config")) {
            const json& hnswConfigJson = denseJson["hnsw_config"];
            
            if (hnswConfigJson.contains("M")) {
                hnswConfig.M = hnswConfigJson["M"];
            } else if (hnswConfigJson.contains("m")) {
                hnswConfig.M = hnswConfigJson["m"];
            } else {
                hnswConfig.M = Config::getInstance().getM();
            }

            hnswConfig.EfConstruct = hnswConfigJson.value("ef_construct", Config::getInstance().getEfConstruction());
        }

        QuantizationConfig quantizationConfig = defaultQuantizationConfig;
        if (denseJson.contains("quantization_config")) {
            const json& quantizationJson = denseJson["quantization_config"];
            quantizationConfig = QuantizationConfig::fromJson(quantizationJson);
        }
        
        createDenseVectorIndex(versionId, defaultDenseIndexName, denseDimension, denseMetric, hnswConfig, quantizationConfig, true);
    } else {
        createDenseVectorIndex(versionId, defaultDenseIndexName, denseDimension, denseMetric, defaultHnswConfig, defaultQuantizationConfig, true);
    }
}

void processSparseConfiguration(const json& parsedJson, int versionId) {
    const std::string& defaultSparseIndexName = Config::getInstance().getDefaultSparseIndexName();

    if (parsedJson.contains(defaultSparseIndexName)) {
        const json& sparseJson = parsedJson[defaultSparseIndexName];

        std::string sparseMetric = sparseJson.value("metric", "cosine");
        std::transform(sparseMetric.begin(), sparseMetric.end(), sparseMetric.begin(), ::tolower);

        createSparseVectorIndex(versionId, defaultSparseIndexName, sparseMetric, true);
    }
}

void processIndexesConfiguration(const json& parsedJson, int versionId) {
    if (!parsedJson.contains("indexes")) {
        return;
    }

    const json& indexesJson = parsedJson["indexes"];
    bool is_default = true;

    for (auto it = indexesJson.begin(); it != indexesJson.end(); ++it) {
        std::string indexName = it.key();
        const json& indexJson = it.value();

        int dimension = indexJson.value("dimension", 0); 
        std::string metric = indexJson.value("metric", "l2");
        std::transform(metric.begin(), metric.end(), metric.begin(), ::tolower);

        HnswConfig hnswConfig;

        if (indexJson.contains("hnsw_config")) {
            const json& hnswConfigJson = indexJson["hnsw_config"];
            if (hnswConfigJson.contains("M")) {
                hnswConfig.M = hnswConfigJson["M"];
            } else if (hnswConfigJson.contains("m")) {
                hnswConfig.M = hnswConfigJson["m"];
            } else {
                hnswConfig.M = Config::getInstance().getM();
            }
            hnswConfig.EfConstruct = hnswConfigJson.value("ef_construct", Config::getInstance().getEfConstruction());
        }

        QuantizationConfig quantizationConfig;
        if (indexJson.contains("quantization_config")) {
            const json& quantizationJson = indexJson["quantization_config"];
            quantizationConfig = QuantizationConfig::fromJson(quantizationJson);
        }

        createDenseVectorIndex(versionId, indexName, dimension, metric, hnswConfig, quantizationConfig, is_default);
        is_default = false;
    }
}

int createDenseVectorIndex(int versionId, const std::string& name, int denseDimension, const std::string& denseMetric,
                      const HnswConfig& hnswConfig, const QuantizationConfig& quantizationConfig, bool is_default) {
    VectorIndex vectorIndex(0, versionId, VectorValueType::Dense, name, metricTypeFromString(denseMetric), denseDimension,
                            hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 0, 0, is_default);
    return VectorIndexManager::getInstance().addVectorIndex(vectorIndex);
}

int createSparseVectorIndex(int versionId, const std::string& name, const std::string& sparseMetric, bool is_default) {
    VectorIndex sparseVectorIndex(0, versionId, VectorValueType::Sparse, name, metricTypeFromString(sparseMetric), 0,
                                  "{}", "{}", 0, 0, is_default);
    return VectorIndexManager::getInstance().addVectorIndex(sparseVectorIndex);
}

nlohmann::json fetchSpaceDetails(const Space& space) {
    nlohmann::json result;

    result["spaceId"] = space.id;
    result["name"] = space.name;
    result["created_time_utc"] = space.created_time_utc;
    result["updated_time_utc"] = space.updated_time_utc;

    auto versions = VersionManager::getInstance().getVersionsBySpaceId(space.id, 0, std::numeric_limits<int>::max());
    if (versions.empty()) {
        spdlog::error("No versions found for spaceId: {}", space.id);
        throw std::runtime_error("No default version found for the specified space.");
    }

    const auto& version = versions[0];
    nlohmann::json versionJson;
    versionJson["versionId"] = version.unique_id;

    auto vectorIndices = VectorIndexManager::getInstance().getVectorIndicesByVersionId(version.id);    
    nlohmann::json vectorIndicesJson = nlohmann::json::array();

    for (const auto& vectorIndex : vectorIndices) {
        spdlog::debug("Processing vectorIndexId: {}, name: {}", vectorIndex.id, vectorIndex.name);
        
        nlohmann::json vectorIndexJson;
        vectorIndexJson["vectorIndexId"] = vectorIndex.id;
        vectorIndexJson["vectorValueType"] = static_cast<int>(vectorIndex.vectorValueType);
        vectorIndexJson["name"] = vectorIndex.name;
        vectorIndexJson["created_time_utc"] = vectorIndex.create_date_utc;
        vectorIndexJson["updated_time_utc"] = vectorIndex.updated_time_utc;
        vectorIndexJson["is_default"] = vectorIndex.is_default;
        vectorIndexJson["metricType"] = static_cast<int>(vectorIndex.metricType);
        vectorIndexJson["dimension"] = vectorIndex.dimension;
        vectorIndexJson["hnswConfig"] = nlohmann::json::parse(vectorIndex.hnswConfigJson);
        vectorIndexJson["quantizationConfig"] = nlohmann::json::parse(vectorIndex.quantizationConfigJson);

        vectorIndicesJson.push_back(vectorIndexJson);
    }

    versionJson["vectorIndices"] = vectorIndicesJson;
    result["version"] = versionJson;

    return result;
}

VectorIndex* getVectorIndexByName(int versionId, const std::string& indexName) {
    auto vectorIndices = VectorIndexManager::getInstance().getVectorIndicesByVersionId(versionId);
    for (auto& idx : vectorIndices) {
        if (idx.name == indexName) {
            return &idx;
        }
    }
    return nullptr;
}

void updateDefaultDenseIndex(const json& parsedJson, int versionId, int spaceId) {
    const std::string& defaultDenseIndexName = Config::getInstance().getDefaultDenseIndexName();

    if (!parsedJson.contains(defaultDenseIndexName)) {
        return;
    }

    const json& denseJson = parsedJson[defaultDenseIndexName];

    std::vector<VectorIndex> vectorIndices = VectorIndexManager::getInstance().getVectorIndicesByVersionId(versionId);
    VectorIndex* denseIndex = nullptr;
    for (auto& idx : vectorIndices) {
        if (idx.name == defaultDenseIndexName) {
            denseIndex = &idx;
            break;
        }
    }
    
    if (denseIndex == nullptr) {
        spdlog::error("Dense vector index '{}' not found for spaceId: {}", defaultDenseIndexName, spaceId);
        throw std::runtime_error("Dense vector index not found.");
    }

    if (denseJson.contains("dimension")) {
        denseIndex->dimension = denseJson["dimension"];
    }
    
    if (denseJson.contains("metric")) {
        denseIndex->metricType = metricTypeFromString(denseJson["metric"].get<std::string>());
    }
    
    if (denseJson.contains("hnsw_config")) {
        json hnswConfigJson = denseJson["hnsw_config"];
        HnswConfig hnswConfig = denseIndex->getHnswConfig();
        if (hnswConfigJson.contains("m")) {
            hnswConfig.M = hnswConfigJson["m"];
        }
        if (hnswConfigJson.contains("ef_construct")) {
            hnswConfig.EfConstruct = hnswConfigJson["ef_construct"];
        }
        denseIndex->setHnswConfig(hnswConfig);
    }

    VectorIndexManager::getInstance().updateVectorIndex(*denseIndex);
}

void updateDefaultSparseIndex(const json& parsedJson, int versionId, int spaceId) {
    const std::string& defaultSparseIndexName = Config::getInstance().getDefaultSparseIndexName();

    if (!parsedJson.contains(defaultSparseIndexName)) {
        return;
    }

    const json& sparseJson = parsedJson[defaultSparseIndexName];

    std::vector<VectorIndex> vectorIndices = VectorIndexManager::getInstance().getVectorIndicesByVersionId(versionId);
    VectorIndex* sparseIndex = nullptr;
    for (auto& idx : vectorIndices) {
        if (idx.name == defaultSparseIndexName) {
            sparseIndex = &idx;
            break;
        }
    }

    if (sparseIndex == nullptr) {
        spdlog::error("Sparse vector index '{}' not found for spaceId: {}", defaultSparseIndexName, spaceId);
        throw std::runtime_error("Sparse vector index not found.");
    }

    if (sparseJson.contains("metric")) {
        sparseIndex->metricType = metricTypeFromString(sparseJson["metric"].get<std::string>());
    }

    VectorIndexManager::getInstance().updateVectorIndex(*sparseIndex);
}

void updateAdditionalIndexes(const json& parsedJson, int versionId, int spaceId) {
    if (!parsedJson.contains("indexes")) {
        return;
    }

    const json& indexesJson = parsedJson["indexes"];
    for (auto it = indexesJson.begin(); it != indexesJson.end(); ++it) {
        std::string indexName = it.key();
        const json& indexJson = it.value();

        VectorIndex* targetIndex = getVectorIndexByName(versionId, indexName);

        if (targetIndex == nullptr) {
            int dimension = indexJson.value("dimension", 0);
            std::string metric = indexJson.value("metric", "l2");
            HnswConfig hnswConfig;
            if (indexJson.contains("hnsw_config")) {
                json hnswConfigJson = indexJson["hnsw_config"];
                hnswConfig.M = hnswConfigJson.value("m", Config::getInstance().getM());
                hnswConfig.EfConstruct = hnswConfigJson.value("ef_construct", Config::getInstance().getEfConstruction());
            }
            QuantizationConfig quantizationConfig;
            if (indexJson.contains("quantization_config")) {
                json quantizationJson = indexJson["quantization_config"];
                quantizationConfig = QuantizationConfig::fromJson(quantizationJson);
            }

            createDenseVectorIndex(versionId, indexName, dimension, metric, hnswConfig, quantizationConfig, false);
        } else {
            if (indexJson.contains("dimension")) {
                targetIndex->dimension = indexJson["dimension"];
            }
            if (indexJson.contains("metric")) {
                targetIndex->metricType = metricTypeFromString(indexJson["metric"].get<std::string>());
            }
            if (indexJson.contains("hnsw_config")) {
                json hnswConfigJson = indexJson["hnsw_config"];
                HnswConfig hnswConfig = targetIndex->getHnswConfig();
                if (hnswConfigJson.contains("m")) {
                    hnswConfig.M = hnswConfigJson["m"];
                }
                if (hnswConfigJson.contains("ef_construct")) {
                    hnswConfig.EfConstruct = hnswConfigJson["ef_construct"];
                }
                targetIndex->setHnswConfig(hnswConfig);
            }
            if (indexJson.contains("quantization_config")) {
                json quantizationJson = indexJson["quantization_config"];
                QuantizationConfig quantizationConfig = targetIndex->getQuantizationConfig();
                quantizationConfig = QuantizationConfig::fromJson(quantizationJson);
                targetIndex->setQuantizationConfig(quantizationConfig);
            }

            VectorIndexManager::getInstance().updateVectorIndex(*targetIndex);
        }
    }
}

} // anonymous namespace

void SpaceServiceManager::createSpace(const std::string& jsonStr) {
    json parsedJson = json::parse(jsonStr);

    std::string spaceName = "";
    std::string spaceDescription = "Automatically created space";

    if (parsedJson.contains("name")) {
        spaceName = parsedJson["name"];
        spaceName.erase(spaceName.find_last_not_of(" \n\r\t") + 1);
        spaceName.erase(0, spaceName.find_first_not_of(" \n\r\t"));
    } else {
        throw std::invalid_argument("Space name is required.");
    }

    if (!std::regex_match(spaceName, std::regex("^[a-zA-Z0-9_-]+$"))) {
        throw std::invalid_argument("Invalid 'name' format, only alphanumeric characters, '_', and '-' are allowed");
    }

    if (parsedJson.contains("description")) {
        spaceDescription = parsedJson["description"];
    }

    // validation
    try {
        int versionId = IdCache::getInstance().getDefaultVersionId(spaceName);
        if (versionId >= 0) {
            spdlog::info("spaceName: {} already exists", spaceName);
            return;
        }
    } catch (const std::exception& e) {
    }

    Space space(0, spaceName, spaceDescription, 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(space);

    Version defaultVersion(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    processDenseConfiguration(parsedJson, versionId);
    processSparseConfiguration(parsedJson, versionId);
    processIndexesConfiguration(parsedJson, versionId);
}

void SpaceServiceManager::deleteSpace(const std::string& spaceName, const std::string& jsonStr) {
    spdlog::info("Starting deleteSpace for spaceName: {}", spaceName);

    try {
        auto space = SpaceManager::getInstance().getSpaceByName(spaceName);
        if (space.id <= 0) {
            spdlog::error("Space with name '{}' not found.", spaceName);
            throw std::runtime_error("Space not found.");
        }

        json parsedJson = json::parse(jsonStr);

        SpaceManager::getInstance().deleteSpace(space.id);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getBySpaceName for spaceName: {}. Error: {}", spaceName, e.what());
        throw;
    }
}

nlohmann::json SpaceServiceManager::getBySpaceId(int spaceId) {
    try {
        auto space = SpaceManager::getInstance().getSpaceById(spaceId);
        if (space.id != spaceId) {
            spdlog::error("Space with spaceId: {} not found.", spaceId);
            throw std::runtime_error("Space not found.");
        }

        return fetchSpaceDetails(space);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getBySpaceId for spaceId: {}. Error: {}", spaceId, e.what());
        throw;
    }
}

nlohmann::json SpaceServiceManager::getBySpaceName(const std::string& spaceName) {
    try {
        auto space = SpaceManager::getInstance().getSpaceByName(spaceName);
        if (space.id <= 0) {
            spdlog::error("Space with name '{}' not found.", spaceName);
            throw std::runtime_error("Space not found.");
        }

        return fetchSpaceDetails(space);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getBySpaceName for spaceName: {}. Error: {}", spaceName, e.what());
        throw;
    }
}

nlohmann::json SpaceServiceManager::getLists() {
    nlohmann::json result;
    nlohmann::json values = nlohmann::json::array();

    auto spaces = SpaceManager::getInstance().getAllSpaces();

    for (const auto& space : spaces) {
        nlohmann::json spaceJson;
        spaceJson["name"] = space.name;
        spaceJson["id"] = space.id;
        spaceJson["description"] = space.description;
        spaceJson["created_time_utc"] = space.created_time_utc;
        spaceJson["updated_time_utc"] = space.updated_time_utc;
        values.push_back(spaceJson);
    }

    result["values"] = values;

    return result;
}

void SpaceServiceManager::updateSpace(const std::string& spaceName, const std::string& jsonStr) {
    spdlog::info("Starting updateSpace for spaceName: {}", spaceName);

    try {
        Space space = SpaceManager::getInstance().getSpaceByName(spaceName);

        if (space.id <= 0) {
            spdlog::error("Space with name '{}' not found.", spaceName);
            throw std::runtime_error("Space not found.");
        }

        json parsedJson = json::parse(jsonStr);
        int versionId = IdCache::getInstance().getDefaultVersionId(spaceName);

        int vectorCount = VectorManager::getInstance().countByVersionId(versionId);
        if (vectorCount > 0) {
            spdlog::error("Cannot update space '{}'. There are {} vectors assigned to its vector indices.", spaceName, vectorCount);
            throw std::runtime_error("Cannot update space: vectors are assigned to vector indices. please cleanup vector index before update");
        }

        updateDefaultDenseIndex(parsedJson, versionId, space.id);
        updateDefaultSparseIndex(parsedJson, versionId, space.id);
        updateAdditionalIndexes(parsedJson, versionId, space.id);

        space.updated_time_utc = getCurrentTimeUTC();
        SpaceManager::getInstance().updateSpace(space);
        spdlog::debug("Updated space's updated_time_utc to {}", space.updated_time_utc);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in updateSpace for spaceName: {}. Error: {}", spaceName, e.what());
        throw;
    }
}

}
}
