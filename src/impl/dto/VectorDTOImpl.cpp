// VectorDTOImpl.cpp
#include "dto/VectorDTO.hpp"
#include "Space.hpp"
#include "Version.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "DatabaseManager.hpp"
#include "SparseDataPool.hpp"
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

            // Determine the type of vector (Dense or Sparse)
            VectorValueType valueType = VectorValueType::Dense; // Default to Dense
            if (vectorJson.contains("sparse_data")) {
                const auto& sparseDataJson = vectorJson["sparse_data"];
                if (sparseDataJson.contains("indices") && sparseDataJson.contains("values")) {
                    valueType = VectorValueType::Sparse;
                }
            }

            Vector vector(vectorId, versionId, unique_id, valueType, {}, false);

            if (vectorJson.contains("data") || vectorJson.contains("sparse_data")) {
                if (valueType == VectorValueType::Dense && vectorJson.contains("data")) {
                    auto dataJson = vectorJson["data"];
                    if (dataJson.is_array()) {
                        VectorValue denseValue(0, vector.id, vectorIndexId, VectorValueType::Dense, dataJson.get<std::vector<float>>());
                        vector.values.push_back(denseValue);
                    } else {
                        throw std::runtime_error("Data format is not supported for Dense vector.");
                    }
                } else if (valueType == VectorValueType::Sparse && vectorJson.contains("sparse_data")) {
                    auto sparseDataJson = vectorJson["sparse_data"];
                    if (sparseDataJson.contains("indices") && sparseDataJson.contains("values")) {
                        if (sparseDataJson["indices"].is_array() && sparseDataJson["values"].is_array()) {
                            // Combine indices and values into SparseData
                            std::vector<std::pair<int, float>> sparseDataPairs;
                            const auto& indices = sparseDataJson["indices"];
                            const auto& values = sparseDataJson["values"];
                            
                            if (indices.size() != values.size()) {
                                throw std::runtime_error("Indices and values arrays must have the same length.");
                            }

                            SparseData* sparseData = idCache.getSparseDataPool(vectorIndexId).allocate();

                            for (size_t i = 0; i < indices.size(); ++i) {
                                sparseData->emplace_back(indices[i].get<int>(), values[i].get<float>());
                            }
                            VectorValue sparseValue(0, vector.id, vectorIndexId, VectorValueType::Sparse, sparseData);
                            vector.values.push_back(sparseValue);
                        } else {
                            throw std::runtime_error("indices and values must be arrays.");
                        }
                    } else {
                        throw std::runtime_error("Sparse vector must contain indices and values.");
                    }
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
            if (!parsedJson["data"].empty() && parsedJson["data"][0].is_object()) {
                // Assuming each element can specify its type
                for (const auto& vectorData : parsedJson["data"]) {
                    VectorValueType valueType = VectorValueType::Dense; // Default
                    if (vectorData.contains("indices") && vectorData.contains("values")) {
                        valueType = VectorValueType::Sparse;
                    }

                    Vector vector(0, versionId, 0, valueType, {}, false);
                    
                    if (valueType == VectorValueType::Dense) {
                        if (vectorData.contains("data")) {
                            VectorValue denseValue(0, vector.id, vectorIndexId, VectorValueType::Dense, vectorData["data"].get<std::vector<float>>());
                            vector.values.push_back(denseValue);
                        } else {
                            throw std::runtime_error("Dense vector must contain 'data' field.");
                        }
                    } else if (valueType == VectorValueType::Sparse) {
                        if (vectorData.contains("indices") && vectorData.contains("values")) {
                            // Combine indices and values into SparseData
                            std::vector<std::pair<int, float>> sparseDataPairs;
                            const auto& indices = vectorData["indices"];
                            const auto& values = vectorData["values"];
                            
                            if (indices.size() != values.size()) {
                                throw std::runtime_error("Indices and values arrays must have the same length.");
                            }

                            SparseData* sparseData = idCache.getSparseDataPool(vectorIndexId).allocate();

                            for (size_t i = 0; i < indices.size(); ++i) {
                                sparseData->emplace_back(indices[i].get<int>(), values[i].get<float>());
                            }
                            VectorValue sparseValue(0, vector.id, vectorIndexId, VectorValueType::Sparse, sparseData);
                            vector.values.push_back(sparseValue);
                        } else {
                            throw std::runtime_error("Sparse vector must contain 'indices' and 'values'.");
                        }
                    }

                    VectorManager::getInstance().addVector(vector);
                }
            } else if (!parsedJson["data"].empty() && parsedJson["data"][0].is_array()) {
                for (const auto& vectorData : parsedJson["data"]) {
                    Vector vector(0, versionId, 0, VectorValueType::Dense, {}, false);
                    vector.values.push_back(VectorValue(0, vector.id, vectorIndexId, VectorValueType::Dense, vectorData.get<std::vector<float>>()));
                    VectorManager::getInstance().addVector(vector);
                }
            } else {
                // Assuming it's a single dense vector
                Vector vector(0, versionId, 0, VectorValueType::Dense, {}, false);
                vector.values.push_back(VectorValue(0, vector.id, vectorIndexId, VectorValueType::Dense, parsedJson["data"].get<std::vector<float>>()));
                VectorManager::getInstance().addVector(vector);
            }
        } else if (parsedJson["data"].is_object()) {
            // Single vector object with type
            VectorValueType valueType = VectorValueType::Dense; // Default
            if (parsedJson["data"].contains("indices") && parsedJson["data"].contains("values")) {
                valueType = VectorValueType::Sparse;
            }

            Vector vector(0, versionId, 0, valueType, {}, false);

            if (valueType == VectorValueType::Dense) {
                if (parsedJson["data"].contains("data")) {
                    vector.values.emplace_back(VectorValue(0, vector.id, vectorIndexId, VectorValueType::Dense, parsedJson["data"]["data"].get<std::vector<float>>()));
                } else {
                    throw std::runtime_error("Dense vector must contain 'data' field.");
                }
            } else if (valueType == VectorValueType::Sparse) {
                if (parsedJson["data"].contains("indices") && parsedJson["data"].contains("values")) {
                    // Combine indices and values into SparseData
                    std::vector<std::pair<int, float>> sparseDataPairs;
                    const auto& indices = parsedJson["data"]["indices"];
                    const auto& values = parsedJson["data"]["values"];
                    
                    if (indices.size() != values.size()) {
                        throw std::runtime_error("Indices and values arrays must have the same length.");
                    }

                    SparseData* sparseData = idCache.getSparseDataPool(vectorIndexId).allocate();

                    for (size_t i = 0; i < indices.size(); ++i) {
                        sparseData->emplace_back(indices[i].get<int>(), values[i].get<float>());
                    }
                    VectorValue sparseValue(0, vector.id, vectorIndexId, VectorValueType::Sparse, sparseData);
                    vector.values.emplace_back(sparseValue);
                } else {
                    throw std::runtime_error("Sparse vector must contain 'indices' and 'values'.");
                }
            }

            VectorManager::getInstance().addVector(vector);
        }
    }
}

void VectorDTOManager::processSimpleVectors(const json& vectorsJson, int versionId, int defaultIndexId) {
    for (const auto& vectorData : vectorsJson) {
        VectorValueType valueType = VectorValueType::Dense; // Default
        if (vectorData.contains("indices") && vectorData.contains("values")) {
            valueType = VectorValueType::Sparse;
        }

        Vector vector(0, versionId, 0, valueType, {}, false);
        
        if (valueType == VectorValueType::Dense) {
            VectorValue denseValue(0, vector.id, defaultIndexId, VectorValueType::Dense, vectorData.get<std::vector<float>>());
            vector.values.push_back(denseValue);
        } else if (valueType == VectorValueType::Sparse) {
            // Combine indices and values into SparseData
            std::vector<std::pair<int, float>> sparseDataPairs;
            const auto& indices = vectorData["indices"];
            const auto& values = vectorData["values"];
            
            if (indices.size() != values.size()) {
                throw std::runtime_error("Indices and values arrays must have the same length.");
            }

            SparseData* sparseData = IdCache::getInstance().getSparseDataPool(defaultIndexId).allocate();
            for (size_t i = 0; i < indices.size(); ++i) {
                sparseData->emplace_back(indices[i].get<int>(), values[i].get<float>());
            }
            VectorValue sparseValue(0, vector.id, defaultIndexId, VectorValueType::Sparse, sparseData);
            vector.values.push_back(sparseValue);
        }

        VectorManager::getInstance().addVector(vector);
    }
}

void VectorDTOManager::processVectors(const json& parsedJson, int versionId, int defaultIndexId) {
    const auto& vectorsJson = parsedJson["vectors"];

    for (const auto& vectorJson : vectorsJson) {
        int vectorId = vectorJson.value("id", 0);

        // Determine the type of vector
        VectorValueType valueType = VectorValueType::Dense; // Default
        if (vectorJson.contains("sparse_data")) {
            const auto& sparseDataJson = vectorJson["sparse_data"];
            if (sparseDataJson.contains("indices") && sparseDataJson.contains("values")) {
                valueType = VectorValueType::Sparse;
            }
        }

        Vector vector(vectorId, versionId, 0, valueType, {}, false); 

        if (vectorJson.contains("data") || vectorJson.contains("sparse_data")) {
            if (valueType == VectorValueType::Dense && vectorJson.contains("data")) {
                auto dataJson = vectorJson["data"];
                if (dataJson.is_array()) {
                    VectorValue denseValue(0, vector.id, defaultIndexId, VectorValueType::Dense, dataJson.get<std::vector<float>>());
                    vector.values.push_back(denseValue);
                } else {
                    throw std::runtime_error("Data format is not supported for Dense vector.");
                }
            } else if (valueType == VectorValueType::Sparse && vectorJson.contains("sparse_data")) {
                auto sparseDataJson = vectorJson["sparse_data"];
                if (sparseDataJson.contains("indices") && sparseDataJson.contains("values")) {
                    if (sparseDataJson["indices"].is_array() && sparseDataJson["values"].is_array()) {
                        // Combine indices and values into SparseData
                        std::vector<std::pair<int, float>> sparseDataPairs;
                        const auto& indices = sparseDataJson["indices"];
                        const auto& values = sparseDataJson["values"];
                        
                        if (indices.size() != values.size()) {
                            throw std::runtime_error("Indices and values arrays must have the same length.");
                        }

                        SparseData* sparseData = IdCache::getInstance().getSparseDataPool(defaultIndexId).allocate();
                        for (size_t i = 0; i < indices.size(); ++i) {
                            sparseData->emplace_back(indices[i].get<int>(), values[i].get<float>());
                        }
                        VectorValue sparseValue(0, vector.id, defaultIndexId, VectorValueType::Sparse, sparseData);
                        vector.values.push_back(sparseValue);
                    } else {
                        throw std::runtime_error("indices and values must be arrays.");
                    }
                } else {
                    throw std::runtime_error("Sparse vector must contain indices and values.");
                }
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
        vectorJson["id"] = vector.unique_id; // Assuming 'id' is 'unique_id'

        json dataJson;
        for (const auto& vectorValue : vector.values) {
            if (vectorValue.type == VectorValueType::Dense) {
                dataJson["data"] = vectorValue.denseData;
            } else if (vectorValue.type == VectorValueType::Sparse) {
                // Split SparseData into indices and values
                std::vector<int> indices;
                std::vector<float> values;
                for (const auto& pair : *vectorValue.sparseData) {
                    indices.push_back(pair.first);
                    values.push_back(pair.second);
                }
                dataJson["sparse_data"]["indices"] = indices;
                dataJson["sparse_data"]["values"] = values;
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
