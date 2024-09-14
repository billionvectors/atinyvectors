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
    HnswIndexManager(const std::string& indexFileName, int dim, int maxElements, MetricType metric);
    ~HnswIndexManager();

    void addVectorData(const std::vector<float>& vectorData, int vectorId);
    void dumpVectorsToIndex(int vectorIndexId);
    void saveIndex();
    void loadIndex();
    std::vector<std::pair<float, hnswlib::labeltype>> search(const std::vector<float>& queryVector, size_t k);
    bool indexNeedsUpdate();

private:
    void setSpace(MetricType metric);
    void setOptimizerSettings(int vectorIndexId);

    std::vector<float> deserializeVector(const std::string& blobData);

public:
    std::string indexFileName;
    int dim;
    int maxElements;
    hnswlib::HierarchicalNSW<float>* index;

private:
    hnswlib::SpaceInterface<float>* space;
    MetricType metricType;
};

};
};

#endif
