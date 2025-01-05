#ifndef __ATINYVECTORS_FAISS_INDEX_MANAGER_HPP__
#define __ATINYVECTORS_FAISS_INDEX_MANAGER_HPP__

#include <vector>
#include <string>
#include <memory>
#include "faiss/Index.h"
#include "faiss/IndexHNSW.h"
#include "faiss/IndexFlat.h"
#include "nlohmann/json.hpp"
#include "ValueType.hpp"

namespace atinyvectors
{
namespace algo
{

class FaissIndexManager {
public:
    FaissIndexManager(
        const std::string& indexFileName, 
        int vectorIndexId, int dim, int maxElements, 
        MetricType metric, VectorValueType valueType, 
        const HnswConfig& hnswConfig, const QuantizationConfig& quantizationConfig);
    ~FaissIndexManager();

    void addVectorData(const std::vector<float>& vectorData, int vectorId);
    std::vector<std::pair<float, int>> search(const std::vector<float>& queryVector, size_t k);

    void addVectorData(SparseData* sparseData, int vectorId);
    std::vector<std::pair<float, int>> search(SparseData* sparseQueryVector, size_t k);

    void restoreVectorsToIndex(bool skipIfIndexLoaded = true);
    void saveIndex();
    void loadIndex();
    
    bool indexNeedsUpdate();

private:
    void setIndex(
        VectorValueType valueType, MetricType metric, 
        const HnswConfig& hnswConfig, const QuantizationConfig& quantizationConfig);
    void setOptimizerSettings();
    std::vector<float> normalizeVector(const std::vector<float>& vector);
    void normalizeSparseVector(SparseData* sparseVector);

public:
    std::string indexFileName;
    int vectorIndexId;
    VectorValueType valueType;
    int dim;
    int maxElements;
    std::unique_ptr<faiss::Index> index;

private:
    MetricType metricType;
    bool indexLoaded;
};

};
};

#endif
