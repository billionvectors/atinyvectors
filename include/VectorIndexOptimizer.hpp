#ifndef __ATINYVECTORS_VECTOR_INDEX_OPTIMIZER_HPP__
#define __ATINYVECTORS_VECTOR_INDEX_OPTIMIZER_HPP__

#include <vector>
#include <mutex>
#include <memory>
#include "nlohmann/json.hpp"
#include "ValueType.hpp"

namespace atinyvectors {

class HnswConfig {
public:
    int M;
    int EfConstruct;

    HnswConfig(int m = 0, int efConstruct = 0)
        : M(m), EfConstruct(efConstruct) {}

    nlohmann::json toJson() const {
        return nlohmann::json{{"M", M}, {"EfConstruct", EfConstruct}};
    }

    static HnswConfig fromJson(const nlohmann::json& j) {
        return HnswConfig(j.at("M").get<int>(), j.at("EfConstruct").get<int>());
    }
};

class ScalarConfig {
public:
    std::string Type;
    float Quantile;
    bool AlwaysRam;

    ScalarConfig(const std::string& type = "", float quantile = 0.0f, bool alwaysRam = false)
        : Type(type), Quantile(quantile), AlwaysRam(alwaysRam) {}

    nlohmann::json toJson() const {
        return nlohmann::json{{"Type", Type}, {"Quantile", Quantile}, {"AlwaysRam", AlwaysRam}};
    }

    static ScalarConfig fromJson(const nlohmann::json& j) {
        return ScalarConfig(j.at("Type").get<std::string>(), j.at("Quantile").get<float>(), j.at("AlwaysRam").get<bool>());
    }
};

class ProductConfig {
public:
    std::string Compression;
    bool AlwaysRam;

    ProductConfig(const std::string& compression = "", bool alwaysRam = false)
        : Compression(compression), AlwaysRam(alwaysRam) {}

    nlohmann::json toJson() const {
        return nlohmann::json{{"Compression", Compression}, {"AlwaysRam", AlwaysRam}};
    }

    static ProductConfig fromJson(const nlohmann::json& j) {
        return ProductConfig(j.at("Compression").get<std::string>(), j.at("AlwaysRam").get<bool>());
    }
};

class QuantizationConfig {
public:
    ScalarConfig Scalar;
    ProductConfig Product;

    QuantizationConfig(const ScalarConfig& scalar = ScalarConfig(), const ProductConfig& product = ProductConfig())
        : Scalar(scalar), Product(product) {}

    nlohmann::json toJson() const {
        return nlohmann::json{{"Scalar", Scalar.toJson()}, {"Product", Product.toJson()}};
    }

    static QuantizationConfig fromJson(const nlohmann::json& j) {
        return QuantizationConfig(
            ScalarConfig::fromJson(j.at("Scalar")),
            ProductConfig::fromJson(j.at("Product"))
        );
    }
};

class VectorIndexOptimizer {
public:
    int id;
    int vectorIndexId;
    MetricType metricType;
    int dimension;
    std::string hnswConfigJson;
    std::string quantizationConfigJson;

    VectorIndexOptimizer()
        : id(0), vectorIndexId(0), metricType(MetricType::L2), dimension(0), 
          hnswConfigJson("{}"), quantizationConfigJson("{}") {}

    VectorIndexOptimizer(int id, int vectorIndexId, MetricType metricType, int dimension, 
                         const std::string& hnswConfigJson = "{}", const std::string& quantizationConfigJson = "{}")
        : id(id), vectorIndexId(vectorIndexId), metricType(metricType), dimension(dimension),
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

class VectorIndexOptimizerManager {
private:
    static std::unique_ptr<VectorIndexOptimizerManager> instance;
    static std::mutex instanceMutex;

    VectorIndexOptimizerManager();

    void createTable();

public:
    VectorIndexOptimizerManager(const VectorIndexOptimizerManager&) = delete;
    VectorIndexOptimizerManager& operator=(const VectorIndexOptimizerManager&) = delete;

    static VectorIndexOptimizerManager& getInstance();

    // CRUD operations for VectorIndexOptimizer objects
    int addOptimizer(VectorIndexOptimizer& optimizer);
    std::vector<VectorIndexOptimizer> getAllOptimizers();
    VectorIndexOptimizer getOptimizerById(int id);
    std::vector<VectorIndexOptimizer> getOptimizerByIndexId(int vectorIndexId);
    void updateOptimizer(const VectorIndexOptimizer& optimizer);
    void deleteOptimizer(int id);
};

} // namespace atinyvectors

#endif
