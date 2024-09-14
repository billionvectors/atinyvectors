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
    long create_date_utc;
    long updated_time_utc;
    bool is_default;

    VectorIndex()
        : id(0), versionId(0), vectorValueType(VectorValueType::Dense), create_date_utc(0), updated_time_utc(0), is_default(false) {}

    VectorIndex(int id, int versionId, VectorValueType vectorValueType, const std::string& name, long create_date_utc, long updated_time_utc, bool is_default = false)
        : id(id), versionId(versionId), vectorValueType(vectorValueType), name(name), create_date_utc(create_date_utc), updated_time_utc(updated_time_utc), is_default(is_default) {}
};


class VectorIndexManager {
private:
    static std::unique_ptr<VectorIndexManager> instance;
    static std::mutex instanceMutex;

    VectorIndexManager();

    void createTable();

public:
    VectorIndexManager(const VectorIndexManager&) = delete;
    VectorIndexManager& operator=(const VectorIndexManager&) = delete;

    static VectorIndexManager& getInstance();

    int addVectorIndex(VectorIndex& vectorIndex);
    std::vector<VectorIndex> getAllVectorIndices();
    VectorIndex getVectorIndexById(int id);
    std::vector<VectorIndex> getVectorIndicesByVersionId(int versionId);
    void updateVectorIndex(const VectorIndex& vectorIndex);
    void deleteVectorIndex(int id);
};

} // namespace atinyvectors

#endif
