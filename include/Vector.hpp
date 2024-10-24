#ifndef __ATINYVECTORS_VECTOR_HPP__
#define __ATINYVECTORS_VECTOR_HPP__

#include "ValueType.hpp"

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <cstdint>
#include <mutex>

namespace atinyvectors {

class VectorValue {
public:
    int id;
    int vectorId;
    int vectorIndexId; 
    VectorValueType type;

    std::vector<float> denseData;
    SparseData* sparseData;
    int size;
    std::vector<std::vector<float>> multiVectorData;

    VectorValue()
        : id(0), vectorId(0), vectorIndexId(0), 
        type(VectorValueType::Dense), size(0), sparseData(nullptr) {}

    VectorValue(int id, int vectorId, int vectorIndexId, VectorValueType type)
        : id(id), vectorId(vectorId), vectorIndexId(vectorIndexId), 
        type(type), size(0), sparseData(nullptr) {}

    // DenseData
    VectorValue(int id, int vectorId, int vectorIndexId, VectorValueType type, const std::vector<float>& denseData)
        : id(id), vectorId(vectorId), vectorIndexId(vectorIndexId), 
        type(type), denseData(denseData), size(0), sparseData(nullptr) {}

    // SparseData
    VectorValue(int id, int vectorId, int vectorIndexId, VectorValueType type, SparseData* sparseData)
        : id(id), vectorId(vectorId), vectorIndexId(vectorIndexId), 
        type(type), sparseData(sparseData), size(0) {}

    // MultiVectorData
    VectorValue(int id, int vectorId, int vectorIndexId, VectorValueType type, int size, const std::vector<std::vector<float>>& multiVectorData)
        : id(id), vectorId(vectorId), vectorIndexId(vectorIndexId), 
        type(type), size(size), multiVectorData(multiVectorData), sparseData(nullptr) {}

    std::vector<uint8_t> serialize() const;
    void deserialize(const std::vector<uint8_t>& blobData);
};

class Vector {
public:
    long id;
    int versionId;
    int unique_id;
    VectorValueType type;
    std::vector<VectorValue> values;
    bool deleted;

    Vector() : id(0), versionId(0), unique_id(0), type(VectorValueType::Dense), deleted(false) {}

    Vector(long id, int versionId, int unique_id, VectorValueType type, const std::vector<VectorValue>& values, bool deleted)
        : id(id), versionId(versionId), unique_id(unique_id), type(type), values(values), deleted(deleted) {}
};

class VectorManager {
private:
    static std::unique_ptr<VectorManager> instance;
    static std::mutex instanceMutex;

    VectorManager();

public:
    VectorManager(const VectorManager&) = delete;
    VectorManager& operator=(const VectorManager&) = delete;

    static VectorManager& getInstance();

    void createTable();

    int addVector(Vector& vector);
    std::vector<Vector> getAllVectors();
    std::vector<Vector> getVectorsByVersionId(int versionId, int start, int limit);

    int countByVersionId(int versionId);
    
    Vector getVectorById(unsigned long long id);
    Vector getVectorByUniqueId(int versionId, int unique_id);

    void updateVector(const Vector& vector);
    void deleteVector(unsigned long long id);
};

} // namespace atinyvectors

#endif // __VECTOR_HPP__
