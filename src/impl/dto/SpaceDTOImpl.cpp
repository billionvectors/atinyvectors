#include "dto/SpaceDTO.hpp"

#include <string>
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include "Space.hpp"
#include "Version.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorIndexOptimizer.hpp"
#include "VectorMetadata.hpp"

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using namespace nlohmann;

namespace atinyvectors
{
namespace dto
{
namespace
{

// Internal headers
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
    int denseDimension = parsedJson.value("dimension", 128);
    std::string denseMetric = parsedJson.value("metric", "cosine");
    std::transform(denseMetric.begin(), denseMetric.end(), denseMetric.begin(), ::tolower);

    HnswConfig defaultHnswConfig;
    if (parsedJson.contains("hnsw_config")) {
        json hnswConfigJson = parsedJson["hnsw_config"];
        
        // Handle both cases for "M" or "m"
        if (hnswConfigJson.contains("M")) {
            defaultHnswConfig.M = hnswConfigJson["M"];
        } else if (hnswConfigJson.contains("m")) {
            defaultHnswConfig.M = hnswConfigJson["m"];
        } else {
            defaultHnswConfig.M = 32;  // Default value for M
        }

        defaultHnswConfig.EfConstruct = hnswConfigJson.value("ef_construct", 200);  // Default EfConstruct to 200
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
            
            // Handle both cases for "M" or "m"
            if (hnswConfigJson.contains("M")) {
                hnswConfig.M = hnswConfigJson["M"];
            } else if (hnswConfigJson.contains("m")) {
                hnswConfig.M = hnswConfigJson["m"];
            } else {
                hnswConfig.M = 32;
            }

            hnswConfig.EfConstruct = hnswConfigJson.value("ef_construct", hnswConfig.EfConstruct);
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

        int dimension = indexJson.value("dimension", 128); // Default to 128 if not provided
        std::string metric = indexJson.value("metric", "cosine");
        std::transform(metric.begin(), metric.end(), metric.begin(), ::tolower);

        HnswConfig hnswConfig;

        // Check for both "M" and "m" keys and apply the correct value
        if (indexJson.contains("hnsw_config")) {
            json hnswConfigJson = indexJson["hnsw_config"];
            if (hnswConfigJson.contains("M")) {
                hnswConfig.M = hnswConfigJson["M"];
            } else if (hnswConfigJson.contains("m")) {
                hnswConfig.M = hnswConfigJson["m"];
            } else {
                hnswConfig.M = 32;  // Default to 32 if not provided
            }
            hnswConfig.EfConstruct = hnswConfigJson.value("ef_construct", 200);
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
    VectorIndex vectorIndex(0, versionId, VectorValueType::Dense, name, 0, 0, is_default);
    int vectorIndexId = VectorIndexManager::getInstance().addVectorIndex(vectorIndex);

    VectorIndexOptimizer denseOptimizer(0, vectorIndexId, metricTypeFromString(denseMetric), denseDimension);
    denseOptimizer.setHnswConfig(hnswConfig);
    denseOptimizer.setQuantizationConfig(quantizationConfig);
    int optimizerId = VectorIndexOptimizerManager::getInstance().addOptimizer(denseOptimizer);

    return vectorIndexId;
}

int createSparseVectorIndex(int versionId, const std::string& name, const std::string& sparseMetric, bool is_default) {
    VectorIndex sparseVectorIndex(0, versionId, VectorValueType::Sparse, name, 0, 0, is_default);
    int sparseVectorIndexId = VectorIndexManager::getInstance().addVectorIndex(sparseVectorIndex);

    VectorIndexOptimizer sparseOptimizer(0, sparseVectorIndexId, metricTypeFromString(sparseMetric), 0);
    int sparseOptimizerId = VectorIndexOptimizerManager::getInstance().addOptimizer(sparseOptimizer);

    return sparseVectorIndexId;
}

nlohmann::json fetchSpaceDetails(const Space& space) {
    spdlog::info("Fetching details for spaceId: {}, name: {}", space.id, space.name);

    nlohmann::json result;

    result["spaceId"] = space.id;
    result["name"] = space.name;
    result["created_time_utc"] = space.created_time_utc;
    result["updated_time_utc"] = space.updated_time_utc;

    spdlog::info("Space created at: {}, updated at: {}", space.created_time_utc, space.updated_time_utc);

    // Fetching versions for the space
    auto versions = VersionManager::getInstance().getVersionsBySpaceId(space.id);
    if (versions.empty()) {
        spdlog::error("No versions found for spaceId: {}", space.id);
        throw std::runtime_error("No default version found for the specified space.");
    }

    const auto& version = versions[0];
    nlohmann::json versionJson;
    versionJson["versionId"] = version.unique_id;

    spdlog::info("Found versionId: {} for spaceId: {}", version.unique_id, space.id);

    // Fetching vector indices for the version
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

        spdlog::debug("VectorIndex details: created_time_utc: {}, updated_time_utc: {}, is_default: {}",
                      vectorIndex.create_date_utc, vectorIndex.updated_time_utc, vectorIndex.is_default);

        // Fetching optimizer for the vector index
        auto optimizers = VectorIndexOptimizerManager::getInstance().getOptimizerByIndexId(vectorIndex.id);
        if (!optimizers.empty()) {
            const auto& optimizer = optimizers[0];

            // Convert hnswConfigJson to a JSON object
            nlohmann::json hnswConfigJson = nlohmann::json::parse(optimizer.hnswConfigJson);

            spdlog::debug("Optimizer HNSW Config for optimizerId: {}: {}", optimizer.id, hnswConfigJson.dump());

            nlohmann::json optimizerJson;
            optimizerJson["optimizerId"] = optimizer.id;
            optimizerJson["hnswConfig"] = hnswConfigJson;
            optimizerJson["metricType"] = static_cast<int>(optimizer.metricType);
            optimizerJson["dimension"] = optimizer.dimension;

            vectorIndexJson["optimizer"] = optimizerJson;
        } else {
            spdlog::warn("No optimizers found for vectorIndexId: {}", vectorIndex.id);
        }

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

    // Create and add Space
    Space space(0, spaceName, spaceDescription, 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(space);

    // Create and add default Version
    Version defaultVersion(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    processDenseConfiguration(parsedJson, versionId);
    processSparseConfiguration(parsedJson, versionId);
    processIndexesConfiguration(parsedJson, versionId);
}

nlohmann::json SpaceDTOManager::getBySpaceId(int spaceId) {
    spdlog::info("Starting getBySpaceId for spaceId: {}", spaceId);

    // Fetch the space by spaceId
    try {
        auto space = SpaceManager::getInstance().getSpaceById(spaceId);
        if (space.id != spaceId) {
            spdlog::error("Space with spaceId: {} not found.", spaceId);
            throw std::runtime_error("Space not found.");
        }

        spdlog::info("Found space with spaceId: {}, name: {}", space.id, space.name);

        // Fetch and return space details
        return fetchSpaceDetails(space);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getBySpaceId for spaceId: {}. Error: {}", spaceId, e.what());
        throw;
    }
}

nlohmann::json SpaceDTOManager::getBySpaceName(const std::string& spaceName) {
    spdlog::info("Starting getBySpaceName for spaceName: {}", spaceName);

    // Fetch the space using the space name
    try {
        auto space = SpaceManager::getInstance().getSpaceByName(spaceName);
        if (space.id <= 0) {
            spdlog::error("Space with name '{}' not found.", spaceName);
            throw std::runtime_error("Space not found.");
        }

        spdlog::info("Found space with name: '{}', spaceId: {}", space.name, space.id);

        // Fetch and return space details
        return fetchSpaceDetails(space);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred in getBySpaceName for spaceName: {}. Error: {}", spaceName, e.what());
        throw;
    }
}

nlohmann::json SpaceDTOManager::getLists() {
    nlohmann::json result;
    nlohmann::json values = nlohmann::json::array();

    // Fetch all spaces
    auto spaces = SpaceManager::getInstance().getAllSpaces();

    // Iterate through the spaces and add them to the JSON array
    for (const auto& space : spaces) {
        nlohmann::json spaceJson;
        spaceJson[space.name] = space.id;
        values.push_back(spaceJson);
    }

    // Create the final result structure
    result["values"] = values;

    return result;
}

}
}
