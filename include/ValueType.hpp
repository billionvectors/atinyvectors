#ifndef __ATINYVECTORS_VECTOR_VALUE_TYPE_HPP__
#define __ATINYVECTORS_VECTOR_VALUE_TYPE_HPP__

#include "nlohmann/json.hpp"

namespace atinyvectors {

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

} // namespace atinyvectors

#endif
