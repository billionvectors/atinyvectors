#include "algo/HnswIndexManager.hpp"
#include "spdlog/spdlog.h"
#include "DatabaseManager.hpp"
#include <fstream>

namespace atinyvectors
{
namespace algo
{

HnswIndexManager::HnswIndexManager(const std::string& indexFileName, int dim, int maxElements, MetricType metric)
    : indexFileName(indexFileName), dim(dim), maxElements(maxElements), space(nullptr) {
    setSpace(metric);
    index = new hnswlib::HierarchicalNSW<float>(space, maxElements);
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
}

void HnswIndexManager::setSpace(MetricType metric) {
    if (space != nullptr) {
        delete space;
        space = nullptr;
    }

    if (metric == MetricType::L2) {
        space = new hnswlib::L2Space(dim);  // L2 distance space
        spdlog::info("Space set successfully with metric: L2");
    } else if (metric == MetricType::InnerProduct) {
        space = new hnswlib::InnerProductSpace(dim);  // Inner product space
        spdlog::info("Space set successfully with metric: Inner Product");
    } else if (metric == MetricType::Cosine) {
        // Cosine similarity can be approximated using L2 normalization.
        space = new hnswlib::L2Space(dim);  // Cosine similarity approximated by L2 normalized vectors
        spdlog::info("Space set successfully with metric: Cosine");
    } else {
        spdlog::error("Unknown metric type provided");
        throw std::invalid_argument("Unknown metric type");
    }
}

void HnswIndexManager::dumpVectorsToIndex(int vectorIndexId) {
    spdlog::info("Starting dumpVectorsToIndex for vectorIndexId: {}", vectorIndexId);

    if (index) {
        delete index;
        index = nullptr;
    }

    setOptimizerSettings(vectorIndexId);

    auto& db = DatabaseManager::getInstance().getDatabase();
    
    spdlog::info("Loading vectors from database for vectorIndexId: {}", vectorIndexId);
    SQLite::Statement query(db, "SELECT data FROM VectorValue WHERE vectorIndexId = ? AND vectorId IN (SELECT id FROM Vector WHERE deleted = 0)");
    query.bind(1, vectorIndexId);

    int id = 0;
    while (query.executeStep()) {
        const std::string& blobData = query.getColumn(0);
        std::vector<float> vectorData = deserializeVector(blobData);

        if (index == nullptr) {
            spdlog::error("Index is not initialized before adding vectorData.");
            throw std::runtime_error("Index not initialized.");
        }

        spdlog::info("Adding vector data with ID: {} to index", id);
        index->addPoint(vectorData.data(), id);
        id++;
    }

    saveIndex();
}

void HnswIndexManager::setOptimizerSettings(int vectorIndexId) {
    spdlog::info("Setting optimizer settings for vectorIndexId: {}", vectorIndexId);

    auto& db = DatabaseManager::getInstance().getDatabase();

    SQLite::Statement query(db, "SELECT hnswConfigJson, metricType FROM VectorIndexOptimizer WHERE vectorIndexId = ?");
    query.bind(1, vectorIndexId);

    if (query.executeStep()) {
        const std::string& hnswConfigJson = query.getColumn(0).getString();
        int metricTypeValue = query.getColumn(1).getInt();  // metricType 값 가져오기

        // JSON 파싱하여 설정 적용
        nlohmann::json hnswConfig = nlohmann::json::parse(hnswConfigJson);

        int M = hnswConfig.value("M", 16);
        int efConstruction = hnswConfig.value("EfConstruct", 200);
        
        // metricType을 MetricType enum으로 변환
        MetricType metricType = static_cast<MetricType>(metricTypeValue);  // metricType 값에 따라 설정

        // 공간 설정
        setSpace(metricType);

        // 인덱스를 새로운 M 값으로 다시 생성
        delete index;  // 기존 인덱스 삭제
        index = new hnswlib::HierarchicalNSW<float>(space, maxElements, M);
        index->ef_construction_ = efConstruction;

        spdlog::info("Index created with M: {}, efConstruction: {}, Metric: {}", M, efConstruction, metricTypeValue);
    } else {
        spdlog::error("Failed to fetch optimizer settings for vectorIndexId: {}", vectorIndexId);
        throw std::runtime_error("Failed to fetch VectorIndexOptimizer settings");
    }
}

bool HnswIndexManager::indexNeedsUpdate() {
    return false;
}

void HnswIndexManager::addVectorData(const std::vector<float>& vectorData, int vectorId) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    spdlog::info("Adding vector to HNSW index for vectorId: {}", vectorId);
    index->addPoint(vectorData.data(), vectorId);
}

void HnswIndexManager::loadIndex() {
    spdlog::info("Attempting to load HNSW index from file: {}", indexFileName);

    std::ifstream indexFile(indexFileName);
    if (indexFile.good()) {
        spdlog::info("HNSW index file found. Loading index from: {}", indexFileName);

        delete index;
        index = new hnswlib::HierarchicalNSW<float>(space, indexFileName);
        spdlog::info("HNSW index successfully loaded from file: {}", indexFileName);
    } else {
        spdlog::warn("HNSW index file not found. Creating a new index.");
        
        delete index;
        index = new hnswlib::HierarchicalNSW<float>(space, maxElements);
        spdlog::info("New HNSW index created with maxElements: {}", maxElements);
    }
}

void HnswIndexManager::saveIndex() {
    spdlog::info("Saving HNSW index to file: {}", indexFileName);
    index->saveIndex(indexFileName);
}

std::vector<float> HnswIndexManager::deserializeVector(const std::string& blobData) {
    std::vector<float> vectorData(blobData.size() / sizeof(float));
    memcpy(vectorData.data(), blobData.data(), blobData.size());
    return vectorData;
}

std::vector<std::pair<float, hnswlib::labeltype>> HnswIndexManager::search(const std::vector<float>& queryVector, size_t k) {
    if (!index || indexNeedsUpdate()) {
        loadIndex();
    }

    spdlog::info("Searching HNSW index with k = {}", k);
    return index->searchKnnCloserFirst(queryVector.data(), k);
}

};
};
