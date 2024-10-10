#include "dto/SpaceDTO.hpp"
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

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using namespace nlohmann;
using namespace atinyvectors::utils;

namespace atinyvectors
{
namespace dto
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
        if (quantizationJson.contains("scalar")) {
            json scalarJson = quantizationJson["scalar"];
            defaultQuantizationConfig.Scalar.Type = scalarJson.value("type", "int8");
            defaultQuantizationConfig.Scalar.Quantile = scalarJson.value("quantile", 0.99f);
            defaultQuantizationConfig.Scalar.AlwaysRam = scalarJson.value("always_ram", true);
        }
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
            if (quantizationJson.contains("scalar")) {
                const json& scalarJson = quantizationJson["scalar"];
                quantizationConfig.Scalar.Type = scalarJson.value("type", quantizationConfig.Scalar.Type);
                quantizationConfig.Scalar.Quantile = scalarJson.value("quantile", quantizationConfig.Scalar.Quantile);
                quantizationConfig.Scalar.AlwaysRam = scalarJson.value("always_ram", quantizationConfig.Scalar.AlwaysRam);
            }
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
            if (quantizationJson.contains("scalar")) {
                const json& scalarJson = quantizationJson["scalar"];
                quantizationConfig.Scalar.Type = scalarJson.value("type", "int8");
                quantizationConfig.Scalar.Quantile = scalarJson.value("quantile", 0.99f);
                quantizationConfig.Scalar.AlwaysRam = scalarJson.value("always_ram", true);
            }
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
    spdlog::info("Fetching details for spaceId: {}, name: {}", space.id, space.name);

    nlohmann::json result;

    result["spaceId"] = space.id;
    result["name"] = space.name;
    result["created_time_utc"] = space.created_time_utc;
    result["updated_time_utc"] = space.updated_time_utc;

    spdlog::info("Space created at: {}, updated at: {}", space.created_time_utc, space.updated_time_utc);

    auto versions = VersionManager::getInstance().getVersionsBySpaceId(space.id);
    if (versions.empty()) {
        spdlog::error("No versions found for spaceId: {}", space.id);
        throw std::runtime_error("No default version found for the specified space.");
    }

    const auto& version = versions[0];
    nlohmann::json versionJson;
    versionJson["versionId"] = version.unique_id;

    spdlog::info("Found versionId: {} for spaceId: {}", version.unique_id, space.id);

    auto vectorIndices = VectorIndexManager::getInstance().getVectorIndicesByVersionId(version.id);
    spdlog::info("Found {} vector indices for versionId: {}", vectorIndices.size(), version.id);
    
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

    spdlog::info("Successfully fetched space details for spaceId: {}", space.id);

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
        spdlog::info("No dense configuration found in update JSON.");
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
    spdlog::info("Successfully updated dense vector index '{}'.", denseIndex->name);
}

void updateDefaultSparseIndex(const json& parsedJson, int versionId, int spaceId) {
    const std::string& defaultSparseIndexName = Config::getInstance().getDefaultSparseIndexName();

    if (!parsedJson.contains(defaultSparseIndexName)) {
        spdlog::info("No sparse configuration found in update JSON.");
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
    spdlog::info("Successfully updated sparse vector index '{}'.", sparseIndex->name);
}

void updateAdditionalIndexes(const json& parsedJson, int versionId, int spaceId) {
    if (!parsedJson.contains("indexes")) {
        spdlog::info("No additional indexes found in update JSON.");
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
                if (quantizationJson.contains("scalar")) {
                    json scalarJson = quantizationJson["scalar"];
                    quantizationConfig.Scalar.Type = scalarJson.value("type", "int8");
                    quantizationConfig.Scalar.Quantile = scalarJson.value("quantile", 0.99f);
                    quantizationConfig.Scalar.AlwaysRam = scalarJson.value("always_ram", true);
                }
            }

            createDenseVectorIndex(versionId, indexName, dimension, metric, hnswConfig, quantizationConfig, false);
            spdlog::info("Successfully created new vector index '{}'.", indexName);
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
                if (quantizationJson.contains("scalar")) {
                    json scalarJson = quantizationJson["scalar"];
                    quantizationConfig.Scalar.Type = scalarJson.value("type", quantizationConfig.Scalar.Type);
                    quantizationConfig.Scalar.Quantile = scalarJson.value("quantile", quantizationConfig.Scalar.Quantile);
                    quantizationConfig.Scalar.AlwaysRam = quantizationJson.value("always_ram", quantizationConfig.Scalar.AlwaysRam);
                }
                targetIndex->setQuantizationConfig(quantizationConfig);
            }

            VectorIndexManager::getInstance().updateVectorIndex(*targetIndex);
            spdlog::info("Successfully updated vector index '{}'.", targetIndex->name);
        }
    }
}

} // anonymous namespace

void SpaceDTOManager::createSpace(const std::string& jsonStr) {
    json parsedJson = json::parse(jsonStr);

    std::string spaceName = "Default Space";
    std::string spaceDescription = "Automatically created space";

    if (parsedJson.contains("name")) {
        spaceName = parsedJson["name"];
    }

    Space space(0, spaceName, spaceDescription, 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(space);

    Version defaultVersion(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    processDenseConfiguration(parsedJson, versionId);
    processSparseConfiguration(parsedJson, versionId);
    processIndexesConfiguration(parsedJson, versionId);
}

void SpaceDTOManager::deleteSpace(const std::string& spaceName, const std::string& jsonStr) {
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

nlohmann::json SpaceDTOManager::getBySpaceId(int spaceId) {
    spdlog::info("Starting getBySpaceId for spaceId: {}", spaceId);

    try {
        auto space = SpaceManager::getInstance().getSpaceById(spaceId);
        if (space.id != spaceId) {
            spdlog::error("Space with spaceId: {} not found.", spaceId);
            throw std::runtime_error("Space not found.");
        }

        spdlog::info("Found space with spaceId: {}, name: {}", space.id, space.name);

        return fetchSpaceDetails(space);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getBySpaceId for spaceId: {}. Error: {}", spaceId, e.what());
        throw;
    }
}

nlohmann::json SpaceDTOManager::getBySpaceName(const std::string& spaceName) {
    spdlog::info("Starting getBySpaceName for spaceName: {}", spaceName);

    try {
        auto space = SpaceManager::getInstance().getSpaceByName(spaceName);
        if (space.id <= 0) {
            spdlog::error("Space with name '{}' not found.", spaceName);
            throw std::runtime_error("Space not found.");
        }

        spdlog::info("Found space with name: '{}', spaceId: {}", space.name, space.id);

        return fetchSpaceDetails(space);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getBySpaceName for spaceName: {}. Error: {}", spaceName, e.what());
        throw;
    }
}

nlohmann::json SpaceDTOManager::getLists() {
    nlohmann::json result;
    nlohmann::json values = nlohmann::json::array();

    auto spaces = SpaceManager::getInstance().getAllSpaces();

    for (const auto& space : spaces) {
        nlohmann::json spaceJson;
        spaceJson[space.name] = space.id;
        values.push_back(spaceJson);
    }

    result["values"] = values;

    return result;
}

void SpaceDTOManager::updateSpace(const std::string& spaceName, const std::string& jsonStr) {
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
            throw std::runtime_error("Cannot update space: vectors are assigned to vector indices.");
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
