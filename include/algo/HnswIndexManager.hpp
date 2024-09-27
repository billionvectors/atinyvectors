#ifndef __ATINYVECTORS_HNSW_INDEX_MANAGER_HPP__
#define __ATINYVECTORS_HNSW_INDEX_MANAGER_HPP__

#include <vector>
#include <string>
#include "hnswlib/hnswlib.h"
#include "nlohmann/json.hpp"
#include "ValueType.hpp"

namespace atinyvectors
{
namespace algo
{

class HnswIndexManager {
public:
    HnswIndexManager(
        const std::string& indexFileName, 
        int vectorIndexId, int dim, int maxElements, 
        MetricType metric, VectorValueType valueType, HnswConfig& hnswConfig);    
    ~HnswIndexManager();

    void addVectorData(const std::vector<float>& vectorData, int vectorId);
    std::vector<std::pair<float, hnswlib::labeltype>> search(const std::vector<float>& queryVector, size_t k);

    void addVectorData(SparseData* sparseData, int vectorId);
    std::vector<std::pair<float, hnswlib::labeltype>> search(SparseData* sparseQueryVector, size_t k);

    void restoreVectorsToIndex();
    void saveIndex();
    void loadIndex();
    
    bool indexNeedsUpdate();

private:
    void setSpace(VectorValueType valueType, MetricType metric);
    void setOptimizerSettings();

    std::vector<float> deserializeVector(const std::string& blobData);

public:
    std::string indexFileName;
    int vectorIndexId;
    VectorValueType valueType;
    int dim;
    int maxElements;
    hnswlib::HierarchicalNSW<float>* index;

private:
    hnswlib::SpaceInterface<float>* space;
    MetricType metricType;
    HnswConfig* hnswConfig;
    bool indexLoaded;
};

};
};

#endif
