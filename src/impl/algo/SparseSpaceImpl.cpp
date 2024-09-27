#include "algo/SparseIpSpace.hpp"
#include "algo/SparseL2Space.hpp"

namespace atinyvectors
{
namespace algo
{

float SparseIpSpace::SparseIp(const void *pVect1v, const void *pVect2v, const void *dim_ptr) {
    const SparseData* pVect1 = static_cast<const SparseData*>(pVect1v);
    const SparseData* pVect2 = static_cast<const SparseData*>(pVect2v);

    float res = 0.0f;

    size_t i = 0, j = 0;
    size_t dim = *(static_cast<const size_t*>(dim_ptr)); // If needed

    while (i < pVect1->size() && j < pVect2->size()) {
        int index1 = (*pVect1)[i].first;
        int index2 = (*pVect2)[j].first;

        if (index1 == index2) {
            res += (*pVect1)[i].second * (*pVect2)[j].second;
            i++;
            j++;
        } else if (index1 < index2) {
            i++;
        } else {
            j++;
        }
    }

    return -res; // For inner product, negative is used to find max similarity in the HNSW framework
}

float SparseL2Space::SparseL2Sqr(const void *pVect1v, const void *pVect2v, const void *dim_ptr) {
    const SparseData* pVect1 = static_cast<const SparseData*>(pVect1v);
    const SparseData* pVect2 = static_cast<const SparseData*>(pVect2v);

    size_t dim = *(static_cast<const size_t*>(dim_ptr));

    float res = 0.0f;

    size_t i = 0, j = 0;

    while (i < pVect1->size() && j < pVect2->size()) {
        int index1 = (*pVect1)[i].first;
        int index2 = (*pVect2)[j].first;
        float value1 = (*pVect1)[i].second;
        float value2 = (*pVect2)[j].second;

        if (index1 == index2) {
            float diff = value1 - value2;
            res += diff * diff;
            i++;
            j++;
        }
        else if (index1 < index2) {
            res += value1 * value1;
            i++;
        }
        else { // index1 > index2
            res += value2 * value2;
            j++;
        }
    }

    while (i < pVect1->size()) {
        int index1 = (*pVect1)[i].first;
        float value1 = (*pVect1)[i].second;
        res += value1 * value1;
        i++;
    }

    while (j < pVect2->size()) {
        int index2 = (*pVect2)[j].first;
        float value2 = (*pVect2)[j].second;
        res += value2 * value2;
        j++;
    }

    return res;
}

}
}