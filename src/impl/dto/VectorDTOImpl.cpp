#include "dto/VectorDTO.hpp"
#include "Space.hpp"
#include "Version.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using namespace atinyvectors;
using namespace atinyvectors::dto;
using namespace nlohmann;

namespace atinyvectors
{
namespace dto
{

void VectorDTOManager::upsert(const std::string& spaceName, int versionUniqueId, const std::string& jsonStr) {
    json parsedJson = json::parse(jsonStr);

    spdlog::debug("Parsing JSON input. SpaceName={}, versionUniqueId={}", spaceName, versionUniqueId);
    spdlog::debug("Json={}", jsonStr);

    IdCache& idCache = IdCache::getInstance();
    int versionId = idCache.getVersionId(spaceName, versionUniqueId);
    int vectorIndexId = idCache.getVectorIndexId(spaceName, versionUniqueId);

    // Process vectors in JSON
    if (parsedJson.contains("vectors") && parsedJson["vectors"].is_array()) {
        const auto& vectorsJson = parsedJson["vectors"];
        for (const auto& vectorJson : vectorsJson) {
            int unique_id = vectorJson.value("id", 0); 
            int vectorId = 0; 

            Vector vector(vectorId, versionId, unique_id, VectorValueType::Dense, {}, false);

            if (vectorJson.contains("data")) {
                auto dataJson = vectorJson["data"];
                if (dataJson.is_array()) {
                    VectorValue denseValue(0, vector.id, vectorIndexId, VectorValueType::Dense, dataJson.get<std::vector<float>>());
                    vector.values.push_back(denseValue);
                } else {
                    throw std::runtime_error("Data format is not supported in this context.");
                }
            }

            int addedVectorId = VectorManager::getInstance().addVector(vector);

            // Add metadata if present
            if (vectorJson.contains("metadata")) {
                VectorMetadataManager::getInstance().deleteVectorMetadataByVectorId(addedVectorId);
                for (const auto& [key, value] : vectorJson["metadata"].items()) {
                    VectorMetadata metadata(0, addedVectorId, key, value.get<std::string>());
                    VectorMetadataManager::getInstance().addVectorMetadata(metadata);
                }
            }
        }
    }

    // Process standalone data
    if (parsedJson.contains("data")) {
        if (parsedJson["data"].is_array()) {
            if (!parsedJson["data"].empty() && parsedJson["data"][0].is_array()) {
                for (const auto& vectorData : parsedJson["data"]) {
                    Vector vector(0, versionId, 0, VectorValueType::Dense, {}, false);
                    vector.values.push_back(VectorValue(0, vector.id, vectorIndexId, VectorValueType::Dense, vectorData.get<std::vector<float>>()));
                    VectorManager::getInstance().addVector(vector);
                }
            } else {
                Vector vector(0, versionId, 0, VectorValueType::Dense, {}, false);
                vector.values.push_back(VectorValue(0, vector.id, vectorIndexId, VectorValueType::Dense, parsedJson["data"].get<std::vector<float>>()));
                VectorManager::getInstance().addVector(vector);
            }
        }
    }
}

void VectorDTOManager::processSimpleVectors(const json& vectorsJson, int versionId, int defaultIndexId) {
    for (const auto& vectorData : vectorsJson) {
        if (vectorData.is_array()) {
            Vector vector(0, versionId, 0, VectorValueType::Dense, {}, false);
            
            VectorValue denseValue(0, vector.id, defaultIndexId, VectorValueType::Dense, vectorData.get<std::vector<float>>());
            vector.values.push_back(denseValue);
            
            VectorManager::getInstance().addVector(vector);
        }
    }
}

void VectorDTOManager::processVectors(const json& parsedJson, int versionId, int defaultIndexId) {
    const auto& vectorsJson = parsedJson["vectors"];

    for (const auto& vectorJson : vectorsJson) {
        int vectorId = vectorJson.value("id", 0);

        Vector vector(vectorId, versionId, 0, VectorValueType::Dense, {}, false); 

        if (vectorJson.contains("data")) {
            auto dataJson = vectorJson["data"];
            if (dataJson.is_array()) {
                VectorValue denseValue(0, vector.id, defaultIndexId, VectorValueType::Dense, dataJson.get<std::vector<float>>());
                vector.values.push_back(denseValue);
            } else {
                throw std::runtime_error("Data format is not supported in this context.");
            }
        }

        int addedVectorId = VectorManager::getInstance().addVector(vector);

        if (vectorJson.contains("metadata")) {
            VectorMetadataManager::getInstance().deleteVectorMetadataByVectorId(addedVectorId);
            
            for (const auto& [key, value] : vectorJson["metadata"].items()) {
                VectorMetadata metadata(0, addedVectorId, key, value.get<std::string>());
                VectorMetadataManager::getInstance().addVectorMetadata(metadata);
            }
        }
    }
}

json VectorDTOManager::getVectorsByVersionId(int versionId) {
    json result;

    auto vectors = VectorManager::getInstance().getVectorsByVersionId(versionId);
    json vectorsJson = json::array();

    for (const auto& vector : vectors) {
        json vectorJson;
        vectorJson["id"] = vector.id;

        json dataJson;
        for (const auto& vectorValue : vector.values) {
            if (vectorValue.type == VectorValueType::Dense) {
                dataJson["dense"] = vectorValue.denseData;
            } else if (vectorValue.type == VectorValueType::Sparse) {
                dataJson["sparse_indices"] = vectorValue.sparseIndices;
                dataJson["sparse_values"] = vectorValue.sparseValues;
            } else if (vectorValue.type == VectorValueType::MultiVector) {
                dataJson["multivector"] = vectorValue.multiVectorData;
            }
        }

        vectorJson["data"] = dataJson;

        auto metadataList = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector.id);
        json metadataJson;

        for (const auto& metadata : metadataList) {
            metadataJson[metadata.key] = metadata.value;
        }

        vectorJson["metadata"] = metadataJson;
        vectorsJson.push_back(vectorJson);
    }

    result["vectors"] = vectorsJson;
    return result;
}

} // namespace dto
} // namespace atinyvectors
