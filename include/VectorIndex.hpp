#ifndef __ATINYVECTORS_VECTOR_INDEX_HPP__
#define __ATINYVECTORS_VECTOR_INDEX_HPP__

#include <string>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <memory>

#include "ValueType.hpp"

namespace atinyvectors {

class VectorIndex  {
public:
    int id;
    int versionId;
    VectorValueType vectorValueType;
    std::string name;
    MetricType metricType;
    int dimension;
    std::string hnswConfigJson;
    std::string quantizationConfigJson;
    long create_date_utc;
    long updated_time_utc;
    bool is_default;

    VectorIndex()
        : id(0), versionId(0), vectorValueType(VectorValueType::Dense), create_date_utc(0),
          updated_time_utc(0), is_default(false), metricType(MetricType::L2),
          dimension(0), hnswConfigJson("{}"), quantizationConfigJson("{}") {}

    VectorIndex(int id, int versionId, VectorValueType vectorValueType, const std::string& name,
                             MetricType metricType, int dimension,
                             const std::string& hnswConfigJson, const std::string& quantizationConfigJson,
                             long create_date_utc, long updated_time_utc, bool is_default)
        : id(id), versionId(versionId), vectorValueType(vectorValueType), name(name),
          create_date_utc(create_date_utc), updated_time_utc(updated_time_utc), is_default(is_default),
          metricType(metricType), dimension(dimension),
          hnswConfigJson(hnswConfigJson), quantizationConfigJson(quantizationConfigJson) {}

    HnswConfig getHnswConfig() const {
        return hnswConfigJson.empty() ? HnswConfig() : HnswConfig::fromJson(nlohmann::json::parse(hnswConfigJson));
    }

    void setHnswConfig(const HnswConfig& config) {
        hnswConfigJson = config.toJson().dump();
    }

    QuantizationConfig getQuantizationConfig() const {
        return quantizationConfigJson.empty() ? QuantizationConfig() : QuantizationConfig::fromJson(nlohmann::json::parse(quantizationConfigJson));
    }

    void setQuantizationConfig(const QuantizationConfig& config) {
        quantizationConfigJson = config.toJson().dump();
    }
};


class VectorIndexManager {
private:
    static std::unique_ptr<VectorIndexManager> instance;
    static std::mutex instanceMutex;

    VectorIndexManager();

public:
    VectorIndexManager(const VectorIndexManager&) = delete;
    VectorIndexManager& operator=(const VectorIndexManager&) = delete;

    static VectorIndexManager& getInstance();
    
    void createTable();

    int addVectorIndex(VectorIndex& vectorIndex);
    std::vector<VectorIndex> getAllVectorIndices();
    VectorIndex getVectorIndexById(int id);
    std::vector<VectorIndex> getVectorIndicesByVersionId(int versionId);
    void updateVectorIndex(VectorIndex& vectorIndex);
    void deleteVectorIndex(int id);
};

} // namespace atinyvectors

#endif
