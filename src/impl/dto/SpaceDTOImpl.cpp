#include "dto/SpaceDTO.hpp"

#include "Space.hpp"
#include "Version.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Config.hpp"

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

    if (parsedJson.contains("dense")) {
        const json& denseJson = parsedJson["dense"];

        denseDimension = denseJson.value("dimension", denseDimension);
        denseMetric = denseJson.value("metric", denseMetric);
        std::transform(denseMetric.begin(), denseMetric.end(), denseMetric.begin(), ::tolower);

        HnswConfig hnswConfig = defaultHnswConfig;
        if (denseJson.contains("hnsw_config")) {
            json hnswConfigJson = denseJson["hnsw_config"];
            
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
            json quantizationJson = denseJson["quantization_config"];
            if (quantizationJson.contains("scalar")) {
                json scalarJson = quantizationJson["scalar"];
                quantizationConfig.Scalar.Type = scalarJson.value("type", quantizationConfig.Scalar.Type);
                quantizationConfig.Scalar.Quantile = scalarJson.value("quantile", quantizationConfig.Scalar.Quantile);
                quantizationConfig.Scalar.AlwaysRam = scalarJson.value("always_ram", quantizationConfig.Scalar.AlwaysRam);
            }
        }
        
        createDenseVectorIndex(versionId, "Default Dense Index", denseDimension, denseMetric, hnswConfig, quantizationConfig, true);
    } else {
        createDenseVectorIndex(versionId, "Default Dense Index", denseDimension, denseMetric, defaultHnswConfig, defaultQuantizationConfig, true);
    }
}

void processSparseConfiguration(const json& parsedJson, int versionId) {
    if (parsedJson.contains("sparse")) {
        const json& sparseJson = parsedJson["sparse"];

        std::string sparseMetric = sparseJson.value("metric", "cosine");
        std::transform(sparseMetric.begin(), sparseMetric.end(), sparseMetric.begin(), ::tolower);

        createSparseVectorIndex(versionId, "Default Sparse Index", sparseMetric, true);
    }
}

void processIndexesConfiguration(const json& parsedJson, int versionId) {
    if (!parsedJson.contains("indexes")) {
        return;
    }

    const json& indexesJson = parsedJson["indexes"];
    
    bool is_default = true; // First vector is default

    for (auto it = indexesJson.begin(); it != indexesJson.end(); ++it) {
        std::string indexName = it.key();
        const json& indexJson = it.value();

        int dimension = indexJson.value("dimension", 0); 
        std::string metric = indexJson.value("metric", "l2");
        std::transform(metric.begin(), metric.end(), metric.begin(), ::tolower);

        HnswConfig hnswConfig;

        if (indexJson.contains("hnsw_config")) {
            json hnswConfigJson = indexJson["hnsw_config"];
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
            json quantizationJson = indexJson["quantization_config"];
            if (quantizationJson.contains("scalar")) {
                json scalarJson = quantizationJson["scalar"];
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

}
}
