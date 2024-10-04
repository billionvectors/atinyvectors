#include "algo/HnswIndexManager.hpp"
#include "algo/SparseIpSpace.hpp"
#include "algo/SparseL2Space.hpp"
#include "Config.hpp"
#include "IdCache.hpp"
#include "Vector.hpp"
#include "DatabaseManager.hpp"
#include <fstream>
#include <cmath>
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace atinyvectors
{
namespace algo
{

HnswIndexManager::HnswIndexManager(
    const std::string& indexFileName, 
    int vectorIndexId, int dim, int maxElements, 
    MetricType metric, VectorValueType valueType, HnswConfig& hnswConfig)
    : indexFileName(indexFileName), vectorIndexId(vectorIndexId), dim(dim), maxElements(maxElements), space(nullptr), indexLoaded(false), valueType(valueType) {
    setSpace(valueType, metric);
    this->hnswConfig = new HnswConfig(hnswConfig.M, hnswConfig.EfConstruct);

    index = nullptr;
}

HnswIndexManager::~HnswIndexManager() {
    if (index != nullptr) {
        delete index;
        index = nullptr;
    }
    if (space != nullptr) {
        delete space;
        space = nullptr;
    }
    if (hnswConfig != nullptr) {
        delete hnswConfig;
        hnswConfig = nullptr;
    }

    IdCache::getInstance().getSparseDataPool(vectorIndexId).clear();
}

void HnswIndexManager::setSpace(VectorValueType valueType, MetricType metric) {
    if (space != nullptr) {
        delete space;
        space = nullptr;
    }

    if (valueType == VectorValueType::Dense) {
        if (metric == MetricType::L2) {
            space = new hnswlib::L2Space(dim);  // L2 distance space for dense vectors
            spdlog::debug("Space set successfully with VectorValueType: Dense, Metric: L2, dim={}", dim);
        } else if (metric == MetricType::InnerProduct) {
            space = new hnswlib::InnerProductSpace(dim);  // Inner product space for dense vectors
            spdlog::debug("Space set successfully with VectorValueType: Dense, Metric: Inner Product, dim={}", dim);
        } else if (metric == MetricType::Cosine) {
            // Cosine similarity can be approximated using InnorProduct normalization.
            space = new hnswlib::InnerProductSpace(dim);  // Cosine similarity approximated by InnorProduct normalized vectors
            spdlog::debug("Space set successfully with VectorValueType: Dense, Metric: Cosine, dim={}", dim);
        } else {
            spdlog::error("Unknown metric type provided for Dense vectors");
            throw std::invalid_argument("Unknown metric type for Dense vectors");
        }
    }
    else if (valueType == VectorValueType::Sparse) {
        // Assuming hnswlib has a SparseSpace implementation
        if (metric == MetricType::L2) {
            space = new SparseL2Space(dim);  // Hypothetical Sparse L2 space
            spdlog::debug("Space set successfully with VectorValueType: Sparse, Metric: L2, dim={}", dim);
        } else if (metric == MetricType::InnerProduct) {
            space = new SparseIpSpace(dim);  // Hypothetical Sparse Inner Product space
            spdlog::debug("Space set successfully with VectorValueType: Sparse, Metric: Inner Product, dim={}", dim);
        } else if (metric == MetricType::Cosine) {
            space = new SparseIpSpace(dim);  // Cosine similarity approximated by Inner Product normalized vectors
            spdlog::debug("Space set successfully with VectorValueType: Sparse, Metric: Cosine, dim={}", dim);
        } else {
            spdlog::error("Unknown metric type provided for Sparse vectors");
            throw std::invalid_argument("Unknown metric type for Sparse vectors");
        }
    }
    else {
        spdlog::error("Unsupported VectorValueType provided");
        throw std::invalid_argument("Unsupported VectorValueType");
    }
}

std::vector<float> HnswIndexManager::normalizeVector(const std::vector<float>& vector) {
    float norm = 0.0f;
    for (float val : vector) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    if (norm == 0.0f) {
        return vector;
    }

    std::vector<float> normalized(vector.size());
    for (size_t i = 0; i < vector.size(); ++i) {
        normalized[i] = vector[i] / norm;
    }
    return normalized;
}

void HnswIndexManager::normalizeSparseVector(SparseData* sparseVector) {
    if (sparseVector == nullptr || sparseVector->empty()) {
        return;
    }

    float norm = 0.0f;
    for (const auto& [idx, val] : *sparseVector) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    if (norm == 0.0f) {
        return;
    }

    for (auto& [idx, val] : *sparseVector) {
        val /= norm;
    }
}

void HnswIndexManager::restoreVectorsToIndex() {
    spdlog::debug("Starting restoreVectorsToIndex for vectorIndexId: {}", vectorIndexId);

    if (index) {
        delete index;
        index = nullptr;
    }

    setOptimizerSettings();

    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Statement query(db, 
        "SELECT V.unique_id, VV.type, VV.data "
        "FROM VectorValue VV "
        "JOIN Vector V ON VV.vectorId = V.id "
        "WHERE VV.vectorIndexId = ? AND V.deleted = 0");
    query.bind(1, vectorIndexId);

    while (query.executeStep()) {
        int unique_id = query.getColumn(0).getInt();
        int typeValue = query.getColumn(1).getInt();
        const void* blobDataPtr = query.getColumn(2).getBlob();
        int blobSize = query.getColumn(2).getBytes();

        std::vector<uint8_t> blobData(reinterpret_cast<const uint8_t*>(blobDataPtr), 
                                     reinterpret_cast<const uint8_t*>(blobDataPtr) + blobSize);

        VectorValue vectorValue;
        vectorValue.type = static_cast<VectorValueType>(typeValue);
        vectorValue.vectorIndexId = vectorIndexId;
        vectorValue.deserialize(blobData);

        if (vectorValue.type == VectorValueType::Dense) {
            if (metricType == MetricType::Cosine) {
                vectorValue.denseData = normalizeVector(vectorValue.denseData);
            }

            index->addPoint(vectorValue.denseData.data(), unique_id);
        } else if (vectorValue.type == VectorValueType::Sparse) {
            if (metricType == MetricType::Cosine) {
                normalizeSparseVector(vectorValue.sparseData);
            }

            index->addPoint(vectorValue.sparseData, unique_id);
        } else {
            spdlog::debug("Unsupported VectorValueType: {}", static_cast<int>(vectorValue.type));
        }
    }

    saveIndex();
}

void HnswIndexManager::setOptimizerSettings() {
    spdlog::debug("Setting optimizer settings for vectorIndexId: {}", vectorIndexId);

    auto& db = DatabaseManager::getInstance().getDatabase();

    // Use VectorIndex table instead of VectorIndexOptimizer
    SQLite::Statement query(db, "SELECT hnswConfigJson, metricType, vectorValueType FROM VectorIndex WHERE id = ?");
    query.bind(1, vectorIndexId);

    if (query.executeStep()) {
        const std::string& hnswConfigJson = query.getColumn(0).getString();
        int metricTypeValue = query.getColumn(1).getInt();
        int vectorValueTypeValue = query.getColumn(2).getInt();

        nlohmann::json hnswConfigJsonParsed;

        // Parse JSON safely and handle empty or invalid JSON
        try {
            hnswConfigJsonParsed = nlohmann::json::parse(hnswConfigJson.empty() ? "{}" : hnswConfigJson);
        } catch (const nlohmann::json::parse_error& e) {
            spdlog::warn("Failed to parse hnswConfigJson: {}. Using default configuration. Error: {}", hnswConfigJson, e.what());
            hnswConfigJsonParsed = nlohmann::json::object();  // Set to empty JSON object if parsing fails
        }

        // Set default values for M and EfConstruct if not found in JSON
        int M = hnswConfigJsonParsed.value("M", Config::getInstance().getM());
        int efConstruction = hnswConfigJsonParsed.value("EfConstruct", Config::getInstance().getEfConstruction());

        MetricType metricType = static_cast<MetricType>(metricTypeValue);
        VectorValueType vectorValueType = static_cast<VectorValueType>(vectorValueTypeValue);
        this->valueType = vectorValueType;  // Update the member variable
        this->metricType = metricType;

        // set new space with the updated valueType and metricType
        setSpace(vectorValueType, metricType);

        // rebuild index with new hnswconfig
        delete index;
        index = new hnswlib::HierarchicalNSW<float>(space, maxElements, M, efConstruction);

        spdlog::debug("Index created with M: {}, efConstruction: {}, Metric: {}, VectorValueType: {}", M, efConstruction, metricTypeValue, vectorValueTypeValue);
    } else {
        spdlog::error("Failed to fetch optimizer settings for vectorIndexId: {}", vectorIndexId);
        throw std::runtime_error("Failed to fetch VectorIndex settings");
    }
}

bool HnswIndexManager::indexNeedsUpdate() {
    return !indexLoaded;
}

void HnswIndexManager::addVectorData(const std::vector<float>& vectorData, int vectorId) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    if (metricType == MetricType::Cosine) {
        index->addPoint(normalizeVector(vectorData).data(), vectorId);
    } else {
        index->addPoint(vectorData.data(), vectorId);
    }
}

void HnswIndexManager::addVectorData(SparseData* vectorData, int vectorId) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    if (metricType == MetricType::Cosine) {
        normalizeSparseVector(vectorData);
    }

    spdlog::debug("Adding vector to HNSW index for vectorId: {}", vectorId);
    index->addPoint(static_cast<const void*>(vectorData), vectorId);
}

void HnswIndexManager::loadIndex() {
    spdlog::debug("Attempting to load HNSW index from file: {}", indexFileName);

    if (index != nullptr) {
        delete index;
        index = nullptr;
    }

    std::ifstream indexFile(indexFileName);
    if (indexFile.good()) {
        spdlog::debug("HNSW index file found. Loading index from: {}", indexFileName);

        index = new hnswlib::HierarchicalNSW<float>(space, indexFileName);
        spdlog::debug("HNSW index successfully loaded from file: {} / count={}", 
            indexFileName, index->getCurrentElementCount());
    } else {
        spdlog::warn("HNSW index file not found. Creating a new index.");
        
        restoreVectorsToIndex();

        spdlog::debug("New HNSW index created with dim: {}", dim);
    }

    indexLoaded = true;
}

void HnswIndexManager::saveIndex() {
    if (valueType != VectorValueType::Dense) {
        spdlog::warn("saveIndex error: currently only support dense type");
        return;
    }

    spdlog::debug("Saving HNSW index to file: {}", indexFileName);

    if (index != nullptr) {
        index->saveIndex(indexFileName);
    }
}

std::vector<std::pair<float, hnswlib::labeltype>> HnswIndexManager::search(const std::vector<float>& queryVector, size_t k) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    spdlog::debug("Searching HNSW index with k = {}", k);

    if (metricType == MetricType::Cosine) {
        return index->searchKnnCloserFirst(normalizeVector(queryVector).data(), k);
    }
    else {
        return index->searchKnnCloserFirst(queryVector.data(), k);
    }
}

std::vector<std::pair<float, hnswlib::labeltype>> HnswIndexManager::search(SparseData* sparseQueryVector, size_t k) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    spdlog::debug("Searching HNSW index with Sparse Vector and k = {}", k);

    if (metricType == MetricType::Cosine) {
        normalizeSparseVector(sparseQueryVector);
    }

    return index->searchKnnCloserFirst(static_cast<const void*>(sparseQueryVector), k);
}

}; // namespace algo
}; // namespace atinyvectors
