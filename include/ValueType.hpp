#ifndef __ATINYVECTORS_VECTOR_VALUE_TYPE_HPP__
#define __ATINYVECTORS_VECTOR_VALUE_TYPE_HPP__

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

} // namespace atinyvectors

#endif
