#include "dto/VectorDTO.hpp"
#include "Space.hpp"
#include "Version.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorIndexOptimizer.hpp"
#include "VectorMetadata.hpp"
#include "DatabaseManager.hpp"
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

void VectorDTOManager::upsert(const std::string& spaceName, int versionId, const std::string& jsonStr) {
    json parsedJson = json::parse(jsonStr);
    auto& db = DatabaseManager::getInstance().getDatabase();

    spdlog::info("Parsing JSON input. SpaceName={}, VersionId={}", spaceName, versionId);
    spdlog::debug("Json={}", jsonStr);

    // If versionId is 0, find the default version for the given space
    if (versionId == 0) {
        SQLite::Statement querySpace(db, "SELECT id FROM Space WHERE name = ?");
        querySpace.bind(1, spaceName);
        int spaceId = -1;

        if (querySpace.executeStep()) {
            spaceId = querySpace.getColumn(0).getInt();
        } else {
            throw std::runtime_error("Space not found with the specified name: " + spaceName);
        }

        SQLite::Statement queryVersion(db, "SELECT id FROM Version WHERE spaceId = ? AND is_default = 1");
        queryVersion.bind(1, spaceId);

        if (queryVersion.executeStep()) {
            versionId = queryVersion.getColumn(0).getInt();
        } else {
            throw std::runtime_error("Default version not found for the specified space: " + spaceName);
        }
    }

    int defaultIndexId = -1;
    std::string selectedIndexName;

    SQLite::Statement queryDefaultIndex(db, "SELECT id, name FROM VectorIndex WHERE versionId = ? AND is_default = 1");
    queryDefaultIndex.bind(1, versionId);

    if (queryDefaultIndex.executeStep()) {
        defaultIndexId = queryDefaultIndex.getColumn(0).getInt();
        selectedIndexName = queryDefaultIndex.getColumn(1).getString();
    } else {
        VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", 0, 0, true);
        defaultIndexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);
        selectedIndexName = defaultIndex.name;
    }

    if (defaultIndexId == -1) {
        throw std::runtime_error("Failed to find or create a default vector index for the specified version.");
    }

    // Process vectors in JSON
    if (parsedJson.contains("vectors") && parsedJson["vectors"].is_array()) {
        const auto& vectorsJson = parsedJson["vectors"];
        for (const auto& vectorJson : vectorsJson) {
            int unique_id = vectorJson.value("id", 0);  // Map id from JSON to unique_id in Vector
            int vectorId = 0;  // Default id is 0; the database assigns a new id if this is a new vector

            Vector vector(vectorId, versionId, unique_id, VectorValueType::Dense, {}, false);

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
                    // Using unique_id = 0 for new vectors without explicit unique_id from JSON
                    Vector vector(0, versionId, 0, VectorValueType::Dense, {}, false);
                    vector.values.push_back(VectorValue(0, vector.id, defaultIndexId, VectorValueType::Dense, vectorData.get<std::vector<float>>()));
                    VectorManager::getInstance().addVector(vector);
                }
            } else {
                // Using unique_id = 0 for new vectors without explicit unique_id from JSON
                Vector vector(0, versionId, 0, VectorValueType::Dense, {}, false);
                vector.values.push_back(VectorValue(0, vector.id, defaultIndexId, VectorValueType::Dense, parsedJson["data"].get<std::vector<float>>()));
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

        // Correct the constructor call to provide unique_id, using 0 as a placeholder
        Vector vector(vectorId, versionId, 0, VectorValueType::Dense, {}, false); // Provide a default value for unique_id

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
