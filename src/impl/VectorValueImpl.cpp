// VectorValueImpl.cpp
#include "Vector.hpp"
#include <cstring>

using namespace atinyvectors;

template <typename T>
void serializeInteger(std::vector<uint8_t>& buffer, T value) {
    uint8_t* rawData = reinterpret_cast<uint8_t*>(&value);
    buffer.insert(buffer.end(), rawData, rawData + sizeof(T));
}

template <typename T>
T deserializeInteger(const uint8_t* data, size_t& offset) {
    T value;
    memcpy(&value, data + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

std::vector<uint8_t> serializeFloatVector(const std::vector<float>& data) {
    std::vector<uint8_t> serializedData(data.size() * sizeof(float));
    memcpy(serializedData.data(), data.data(), serializedData.size());
    return serializedData;
}

std::vector<float> deserializeFloatVector(const std::vector<uint8_t>& blobData, size_t& offset, size_t count) {
    std::vector<float> data(count);
    memcpy(data.data(), blobData.data() + offset, count * sizeof(float));
    offset += count * sizeof(float);
    return data;
}

std::vector<uint8_t> VectorValue::serialize() const {
    std::vector<uint8_t> blobData;
    
    if (type == VectorValueType::Dense) {
        blobData = serializeFloatVector(denseData);
    } else if (type == VectorValueType::Sparse) {
        serializeInteger(blobData, static_cast<int>(sparseIndices.size()));
        blobData.insert(blobData.end(), (uint8_t*)sparseIndices.data(), (uint8_t*)sparseIndices.data() + sparseIndices.size() * sizeof(int));
        blobData.insert(blobData.end(), (uint8_t*)sparseValues.data(), (uint8_t*)sparseValues.data() + sparseValues.size() * sizeof(float));
    } else if (type == VectorValueType::MultiVector) {
        serializeInteger(blobData, size);
        for (const auto& vec : multiVectorData) {
            std::vector<uint8_t> serializedVec = serializeFloatVector(vec);
            blobData.insert(blobData.end(), serializedVec.begin(), serializedVec.end());
        }
    }

    return blobData;
}

void VectorValue::deserialize(const std::vector<uint8_t>& blobData) {
    size_t offset = 0;

    if (type == VectorValueType::Dense) {
        denseData = deserializeFloatVector(blobData, offset, blobData.size() / sizeof(float));
    } else if (type == VectorValueType::Sparse) {
        int indicesCount = deserializeInteger<int>(blobData.data(), offset);
        sparseIndices.resize(indicesCount);
        sparseValues.resize(indicesCount);
        memcpy(sparseIndices.data(), blobData.data() + offset, indicesCount * sizeof(int));
        offset += indicesCount * sizeof(int);
        memcpy(sparseValues.data(), blobData.data() + offset, indicesCount * sizeof(float));
    } else if (type == VectorValueType::MultiVector) {
        size = deserializeInteger<int>(blobData.data(), offset);
        size_t totalFloats = (blobData.size() - offset) / sizeof(float);
        size_t vectorSize = totalFloats / size;
        multiVectorData.resize(size, std::vector<float>(vectorSize));
        for (int i = 0; i < size; ++i) {
            multiVectorData[i] = deserializeFloatVector(blobData, offset, vectorSize);
        }
    }
}
