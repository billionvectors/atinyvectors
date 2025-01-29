#ifndef __ATINYVECTORS_VECTOR_METADATA_HPP__
#define __ATINYVECTORS_VECTOR_METADATA_HPP__

#include <string>
#include <iostream>
#include <vector>
#include <mutex>
#include <memory>

namespace atinyvectors {

class VectorMetadata {
public:
    long id;
    long versionId;
    long vectorId;

    std::string key;
    std::string value;

    VectorMetadata()
        : id(0), versionId(0), vectorId(0), key(""), value("") {}

    VectorMetadata(long id, long versionId, long vectorId, const std::string& key, const std::string& value)
        : id(id), versionId(versionId), vectorId(vectorId), key(key), value(value) {}
};

struct VectorMetadataResult {
    int totalCount;
    std::vector<int> vectorUniqueIds;
};

class VectorMetadataManager {
private:
    static std::unique_ptr<VectorMetadataManager> instance;
    static std::mutex instanceMutex;

    VectorMetadataManager();

public:
    VectorMetadataManager(const VectorMetadataManager&) = delete;
    VectorMetadataManager& operator=(const VectorMetadataManager&) = delete;

    static VectorMetadataManager& getInstance();

    long addVectorMetadata(VectorMetadata& metadata);
    std::vector<VectorMetadata> getAllVectorMetadata();
    VectorMetadata getVectorMetadataById(long id);
    std::vector<VectorMetadata> getVectorMetadataByVectorId(long vectorId);
    void updateVectorMetadata(const VectorMetadata& metadata);
    void deleteVectorMetadata(long id);
    void deleteVectorMetadataByVectorId(long vectorId);

    std::vector<std::pair<float, int>> filterVectors(
        const std::vector<std::pair<float, int>>& inputVectors,
        const std::string& filter);

    VectorMetadataResult queryVectors(
        long versionId, const std::string& filter, int start, int limit);

};

} // namespace atinyvectors

#endif
