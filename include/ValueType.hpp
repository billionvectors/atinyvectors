#ifndef __ATINYVECTORS_VECTOR_VALUE_TYPE_HPP__
#define __ATINYVECTORS_VECTOR_VALUE_TYPE_HPP__

#include "nlohmann/json.hpp"

namespace atinyvectors {

typedef std::vector<std::pair<int, float>> SparseData;

enum class VectorValueType {
    Dense,
    Sparse,
    MultiVector,
    Combined
};

enum class MetricType {
    L2,
    Cosine,
    InnerProduct
};

enum class QuantizationType {
    NoQuantization,
    Scalar,
    Product
};

class HnswConfig {
public:
    int M;
    int EfConstruct;

    HnswConfig(int m = 16, int efConstruct = 100)
        : M(m), EfConstruct(efConstruct) {}

    nlohmann::json toJson() const {
        return nlohmann::json{{"M", M}, {"EfConstruct", EfConstruct}};
    }

    static HnswConfig fromJson(const nlohmann::json& j) {
        int m = j.value("M", 16); // Defaut value is 16
        int efConstruct = j.value("EfConstruct", 100);  // Defaut value is 100

        return HnswConfig(m, efConstruct);
    }
};

class ScalarConfig {
public:
    std::string Type; // int8, uint8, fp16, int4

    ScalarConfig(const std::string& type = "")
        : Type(type) {}

    nlohmann::json toJson() const {
        return nlohmann::json{{"type", Type}};
    }

    static ScalarConfig fromJson(const nlohmann::json& j) {
        return ScalarConfig(j.at("type").get<std::string>());
    }
};

class ProductConfig {
public:
    std::string Compression;

    ProductConfig(const std::string& compression = "")
        : Compression(compression) {}

    nlohmann::json toJson() const {
        return nlohmann::json{{"compression", Compression}};
    }

    static ProductConfig fromJson(const nlohmann::json& j) {
        return ProductConfig(j.at("compression").get<std::string>());
    }
};

class QuantizationConfig {
public:
    ScalarConfig Scalar;
    ProductConfig Product;
    QuantizationType QuantizationType;

    QuantizationConfig(const ScalarConfig& scalar = ScalarConfig(), const ProductConfig& product = ProductConfig())
        : Scalar(scalar), Product(product), QuantizationType(QuantizationType::NoQuantization) {}

    nlohmann::json toJson() const {
        nlohmann::json j;

        switch (QuantizationType) {
            case QuantizationType::NoQuantization:
                return j;

            case QuantizationType::Scalar:
                j["scalar"] = Scalar.toJson();
                break;

            case QuantizationType::Product:
                j["product"] = Product.toJson();
                break;
        }

        return j;
    }

    static QuantizationConfig fromJson(const nlohmann::json& j) {
        QuantizationConfig config;

        if (j.empty()) {
            config.QuantizationType = QuantizationType::NoQuantization;
            return config;
        }

        if (j.contains("scalar")) {
            config.Scalar = ScalarConfig::fromJson(j.at("scalar"));
            config.QuantizationType = QuantizationType::Scalar;
        } else if (j.contains("product")) {
            config.Product = ProductConfig::fromJson(j.at("product"));
            config.QuantizationType = QuantizationType::Product;
        } else {
            config.QuantizationType = QuantizationType::NoQuantization;
        }

        return config;
    }
};

} // namespace atinyvectors

#endif
