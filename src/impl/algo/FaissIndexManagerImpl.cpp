#include <fstream>
#include <cmath>

#include "algo/FaissIndexManager.hpp"
#include "Config.hpp"
#include "IdCache.hpp"
#include "Vector.hpp"
#include "DatabaseManager.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "faiss/index_io.h"
#include "faiss/IndexIDMap.h"

namespace atinyvectors
{
namespace algo
{

FaissIndexManager::FaissIndexManager(
    const std::string& indexFileName, 
    int vectorIndexId, int dim, int maxElements, 
    MetricType metric, VectorValueType valueType, 
    const HnswConfig& hnswConfig, const QuantizationConfig& quantizationConfig)
    : indexFileName(indexFileName), vectorIndexId(vectorIndexId), dim(dim), maxElements(maxElements), 
      metricType(metric), valueType(valueType),
      indexLoaded(false) {
    setIndex(valueType, metric, hnswConfig, quantizationConfig);
}

FaissIndexManager::~FaissIndexManager() {
    IdCache::getInstance().getSparseDataPool(vectorIndexId).clear();
}

void FaissIndexManager::setIndex(
    VectorValueType valueType, MetricType metric, 
    const HnswConfig& hnswConfig, 
    const QuantizationConfig& quantizationConfig) {
    if (index) {
        index.reset();
    }

    this->valueType = valueType;

    // Choose the appropriate metric
    faiss::MetricType faissMetric;
    switch(metric) {
        case MetricType::L2:
            faissMetric = faiss::METRIC_L2;
            break;
        case MetricType::InnerProduct:
            faissMetric = faiss::METRIC_INNER_PRODUCT;
            break;
        case MetricType::Cosine:
            faissMetric = faiss::METRIC_INNER_PRODUCT; // FAISS does not have cosine, use inner product with normalized vectors
            break;
        default:
            spdlog::error("Unknown metric type provided");
            throw std::invalid_argument("Unknown metric type");
    }

    faiss::Index* baseIndex = nullptr;

    // Configure quantization based on QuantizationType
    switch (quantizationConfig.QuantizationType) {
        case QuantizationType::NoQuantization:
            baseIndex = new faiss::IndexHNSWFlat(dim, hnswConfig.M, faissMetric);
            break;

        case QuantizationType::Scalar: {
            faiss::ScalarQuantizer::QuantizerType quantizerType;
            if (quantizationConfig.Scalar.Type == "int8") {
                quantizerType = faiss::ScalarQuantizer::QT_8bit_uniform;
            } else if (quantizationConfig.Scalar.Type == "uint8") {
                quantizerType = faiss::ScalarQuantizer::QT_8bit_direct;
            }  else if (quantizationConfig.Scalar.Type == "int4") {
                quantizerType = faiss::ScalarQuantizer::QT_4bit;
            } else if (quantizationConfig.Scalar.Type == "fp16") {
                quantizerType = faiss::ScalarQuantizer::QT_fp16;
            } else {
                spdlog::error("Unsupported scalar quantization type: {}. Fallback: No Quantization", quantizationConfig.Scalar.Type);
                baseIndex = new faiss::IndexHNSWFlat(dim, hnswConfig.M, faissMetric);
            }

            if (baseIndex == nullptr) {
                baseIndex = new faiss::IndexScalarQuantizer(dim, quantizerType, faissMetric);

                // Prepare dummy data for training
                std::vector<float> trainingData(dim * 100); // 100 dummy vectors
                for (size_t i = 0; i < trainingData.size(); ++i) {
                    trainingData[i] = static_cast<float>(std::rand()) / RAND_MAX;
                }

                // Train the quantizer with dummy data
                baseIndex->train(100, trainingData.data()); // Training with 100 vectors
            }
            break;
        }

        case QuantizationType::Product: {
            // TODO: We should test this option more...
            int m = 4; // Adjust m as per requirement or add it to the ProductConfig structure
            baseIndex = new faiss::IndexPQ(dim, m, 8, faissMetric); // 8 bits per sub-vector
            break;
        }
    }

    if (!baseIndex) {
        spdlog::error("Failed to create base index due to invalid quantization configuration");
        throw std::runtime_error("Invalid quantization configuration");
    }

    faiss::IndexHNSWFlat* hnswIndex = dynamic_cast<faiss::IndexHNSWFlat*>(baseIndex);
    if (hnswIndex) {
        hnswIndex->hnsw.efConstruction = hnswConfig.EfConstruct;
        hnswIndex->hnsw.efSearch = hnswConfig.EfConstruct;
    }

    // Wrap with IndexIDMap for external ID mapping
    faiss::IndexIDMap* idMapIndex = new faiss::IndexIDMap(baseIndex);
    index.reset(idMapIndex);

    spdlog::debug("FAISS index created with Quantization: {}, M: {}, efConstruction: {}, Metric: {}, VectorValueType: {}, ntotal: {}", 
        static_cast<int>(quantizationConfig.QuantizationType), hnswConfig.M, hnswConfig.EfConstruct, static_cast<int>(metric), 
        static_cast<int>(valueType), baseIndex->ntotal);
}

std::vector<float> FaissIndexManager::normalizeVector(const std::vector<float>& vector) {
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

void FaissIndexManager::normalizeSparseVector(SparseData* sparseVector) {
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

void FaissIndexManager::restoreVectorsToIndex(bool skipIfIndexLoaded) {
    if (skipIfIndexLoaded && index != nullptr && index->ntotal > 0) {
        return;
    }

    spdlog::debug("Starting restoreVectorsToIndex for vectorIndexId: {}", vectorIndexId);

    // Initialize index settings
    setOptimizerSettings();

    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Statement query(db, 
        "SELECT V.unique_id, VV.type, VV.data "
        "FROM VectorValue VV "
        "JOIN Vector V ON VV.vectorId = V.id "
        "WHERE VV.vectorIndexId = ? AND V.deleted = 0");
    query.bind(1, vectorIndexId);

    std::vector<float> denseVectors;
    std::vector<faiss::idx_t> vectorIds;
    std::vector<std::vector<float>> sparseVectors; // Handle sparse vectors separately if needed

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
            std::vector<float> vector = vectorValue.denseData;
            if (metricType == MetricType::Cosine) {
                vector = normalizeVector(vector);
            }

            if (vector.size() != this->dim) {
                spdlog::debug("Vector size desn't match with dim: {}", static_cast<int>(vector.size()));
                continue;
            }

            denseVectors.insert(denseVectors.end(), vector.begin(), vector.end());
            vectorIds.push_back(unique_id);
        } 
        else if (vectorValue.type == VectorValueType::Sparse) {
            SparseData* sparseVectorPtr = vectorValue.sparseData;
            if (!sparseVectorPtr) {
                spdlog::warn("Sparse vector is null for vectorId: {}", unique_id);
                continue;
            }
            if (metricType == MetricType::Cosine) {
                normalizeSparseVector(sparseVectorPtr);
            }

            // Convert SparseData to dense vector
            std::vector<float> denseVector(dim, 0.0f);
            for (const auto& [idx, val] : *sparseVectorPtr) {
                if (idx >= 0 && idx < dim) {
                    denseVector[idx] = val;
                }
            }

            denseVectors.insert(denseVectors.end(), denseVector.begin(), denseVector.end());
            vectorIds.push_back(unique_id);
        } 
        else {
            spdlog::debug("Unsupported VectorValueType: {}", static_cast<int>(vectorValue.type));
        }
    }

    if (!denseVectors.empty()) {
        faiss::IndexIDMap* idMapIndex = dynamic_cast<faiss::IndexIDMap*>(index.get());
        if (!idMapIndex) {
            spdlog::error("Index is not of type IndexIDMap");
            throw std::runtime_error("Incorrect index type");
        }

        // FAISS expects the data in contiguous memory (n * d)
        // Ensure that denseVectors has n*d elements
        size_t n = vectorIds.size();
        size_t d = dim;
        if (denseVectors.size() != n * d) {
            spdlog::error("Dense vectors size mismatch: expected {}, got {}", n * d, denseVectors.size());
            throw std::runtime_error("Dense vectors size mismatch");
        }

        idMapIndex->add_with_ids(n, denseVectors.data(), vectorIds.data());

        spdlog::debug("Added {} dense vectors to FAISS HNSW index", vectorIds.size());
    }

    saveIndex();
}

void FaissIndexManager::setOptimizerSettings() {
    spdlog::debug("Setting optimizer settings for vectorIndexId: {}", vectorIndexId);

    auto& db = DatabaseManager::getInstance().getDatabase();

    SQLite::Statement query(db, "SELECT hnswConfigJson, quantizationConfigJson, metricType, vectorValueType FROM VectorIndex WHERE id = ?");
    query.bind(1, vectorIndexId);

    if (query.executeStep()) {
        const std::string& hnswConfigJson = query.getColumn(0).getString();
        const std::string& quantizationConfigJson = query.getColumn(1).getString();
        int metricTypeValue = query.getColumn(2).getInt();
        int vectorValueTypeValue = query.getColumn(3).getInt();

        nlohmann::json hnswConfigJsonParsed;

        // Parse JSON safely and handle empty or invalid JSON
        try {
            hnswConfigJsonParsed = nlohmann::json::parse(hnswConfigJson.empty() ? "{}" : hnswConfigJson);
        } catch (const nlohmann::json::parse_error& e) {
            spdlog::warn("Failed to parse hnswConfigJson: {}. Using default configuration. Error: {}", hnswConfigJson, e.what());
            hnswConfigJsonParsed = nlohmann::json::object();  // Set to empty JSON object if parsing fails
        }

        nlohmann::json quantizationConfigJsonParsed;

        // Parse JSON safely and handle empty or invalid JSON
        try {
            quantizationConfigJsonParsed = nlohmann::json::parse(quantizationConfigJson.empty() ? "{}" : quantizationConfigJson);
        } catch (const nlohmann::json::parse_error& e) {
            spdlog::warn("Failed to parse quantizationConfigJson: {}. Using default configuration. Error: {}", hnswConfigJson, e.what());
            quantizationConfigJsonParsed = nlohmann::json::object();  // Set to empty JSON object if parsing fails
        }

        // Set default values for M and EfConstruct if not found in JSON
        int M = hnswConfigJsonParsed.value("M", Config::getInstance().getM());
        int efConstruction = hnswConfigJsonParsed.value("EfConstruct", Config::getInstance().getEfConstruction());
        int efSearch = hnswConfigJsonParsed.value("EfSearch", Config::getInstance().getEfConstruction());

        metricType = static_cast<MetricType>(metricTypeValue);
        valueType = static_cast<VectorValueType>(vectorValueTypeValue);

        HnswConfig hnswConfig = HnswConfig::fromJson(hnswConfigJsonParsed);
        QuantizationConfig quantizationConfig = QuantizationConfig::fromJson(quantizationConfigJsonParsed);

        // Initialize the index with updated settings
        setIndex(valueType, metricType, hnswConfig, quantizationConfig);

        spdlog::debug("FAISS HNSW index initialized with M: {}, efConstruction: {}, efSearch: {}, Metric: {}, VectorValueType: {}", 
            M, efConstruction, efSearch, metricTypeValue, vectorValueTypeValue);
    } else {
        spdlog::error("Failed to fetch optimizer settings for vectorIndexId: {}", vectorIndexId);
        throw std::runtime_error("Failed to fetch VectorIndex settings");
    }
}

bool FaissIndexManager::indexNeedsUpdate() {
    return !indexLoaded;
}

void FaissIndexManager::addVectorData(const std::vector<float>& vectorData, int vectorId) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    if (index->d != this->dim) {
        spdlog::error("Dimension mismatch: Index d = {}, FaissIndexManager dim = {}", index->d, this->dim);
        throw std::runtime_error(fmt::format("Dimension mismatch: Index d = {}, FaissIndexManager dim = {}", index->d, this->dim));
    }

    std::vector<float> vector = vectorData;
    if (metricType == MetricType::Cosine) {
        vector = normalizeVector(vectorData);
    }

    faiss::IndexIDMap* idMapIndex = dynamic_cast<faiss::IndexIDMap*>(index.get());
    if (!idMapIndex) {
        spdlog::error("Index is not of type IndexIDMap");
        throw std::runtime_error("Incorrect index type");
    }

    // Reshape the vector to be 1 x dim
    const float* x = vector.data();
    faiss::idx_t xids = vectorId;

    idMapIndex->add_with_ids(1, x, &xids);
    spdlog::debug("ntotal: {}", idMapIndex->ntotal);
}

void FaissIndexManager::addVectorData(SparseData* vectorData, int vectorId) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    if (metricType == MetricType::Cosine) {
        normalizeSparseVector(vectorData);
    }

    // Convert SparseData to dense vector
    std::vector<float> denseVector(dim, 0.0f);
    for (const auto& [idx, val] : *vectorData) {
        if (idx >= 0 && idx < dim) {
            denseVector[idx] = val;
        }
    }

    addVectorData(denseVector, vectorId);
}

void FaissIndexManager::loadIndex() {
    spdlog::debug("Attempting to load FAISS index from file: {}", indexFileName);

    if (index) {
        index.reset();
    }

    std::ifstream indexFile(indexFileName, std::ios::binary);
    if (indexFile.good()) {
        spdlog::debug("FAISS index file found. Loading index from: {}", indexFileName);

        // FAISS provides read_index for loading indices
        faiss::Index* loadedIndex = faiss::read_index(indexFileName.c_str());
        if (!loadedIndex) {
            spdlog::error("Failed to load FAISS index from file: {}", indexFileName);
            throw std::runtime_error("Failed to load FAISS index");
        }

        index.reset(loadedIndex);
        spdlog::debug("FAISS index successfully loaded from file: {} / count={}", 
            indexFileName, index->ntotal);
    } else {
        spdlog::warn("FAISS index file not found. Creating a new index.");
        
        restoreVectorsToIndex();

        spdlog::debug("New FAISS HNSW index created with dim: {}", dim);
    }

    indexLoaded = true;
}

void FaissIndexManager::saveIndex() {
    spdlog::debug("Saving FAISS index to file: {}", indexFileName);

    std::filesystem::path indexPath(indexFileName);
    if (indexPath.has_parent_path()) {
        std::filesystem::path directory = indexPath.parent_path();

        if (!std::filesystem::exists(directory)) {
            std::filesystem::create_directories(directory);
        }
    } else {
        indexPath = "./" / indexPath;
        indexFileName = indexPath.string();
    }

    std::ofstream ofs(indexFileName);
    ofs.close();

    if (index) {
        faiss::write_index(index.get(), indexFileName.c_str());
    }
}

std::vector<std::pair<float, int>> FaissIndexManager::search(const std::vector<float>& queryVector, size_t k) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    std::vector<float> vector = queryVector;
    if (metricType == MetricType::Cosine) {
        vector = normalizeVector(queryVector);
    }

    // FAISS expects queries as a 2D array (nq x dim)
    const float* x = vector.data();
    faiss::idx_t nq = 1;
    faiss::idx_t knn = k;

    std::vector<faiss::idx_t> indices(knn);
    std::vector<float> distances(knn);

    faiss::IndexIDMap* idMapIndex = dynamic_cast<faiss::IndexIDMap*>(index.get());
    if (!idMapIndex) {
        spdlog::error("Index is not of type IndexIDMap");
        throw std::runtime_error("Incorrect index type");
    }

    idMapIndex->search(nq, x, knn, distances.data(), indices.data());

    std::vector<std::pair<float, int>> results;
    results.reserve(k);
    for (size_t i = 0; i < knn; ++i) {
        faiss::idx_t faissId = indices[i];
        if (faissId < 0) {
            continue;
        }
        // faissId is your external vectorId
        int originalId = static_cast<int>(faissId);
        results.emplace_back(distances[i], originalId);
    }

    return results;
}

std::vector<std::pair<float, int>> FaissIndexManager::search(SparseData* sparseQueryVector, size_t k) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    if (metricType == MetricType::Cosine) {
        normalizeSparseVector(sparseQueryVector);
    }

    // Convert SparseData to dense vector
    std::vector<float> denseVector(dim, 0.0f);
    for (const auto& [idx, val] : *sparseQueryVector) {
        if (idx >= 0 && idx < dim) {
            denseVector[idx] = val;
        }
    }

    return search(denseVector, k);
}

}; // namespace algo
}; // namespace atinyvectors
