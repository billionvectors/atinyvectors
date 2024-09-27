#ifndef __ATINYVECTORS_SPARSE_IPSPACE_HPP__
#define __ATINYVECTORS_SPARSE_IPSPACE_HPP__

#include "hnswlib/hnswlib.h"
#include <vector>
#include <algorithm>
#include <cmath>

#include "ValueType.hpp"

namespace atinyvectors
{
namespace algo
{

class SparseIpSpace : public hnswlib::SpaceInterface<float> {
    hnswlib::DISTFUNC<float> fstdistfunc_;
    size_t data_size_;
    size_t dim_;

public:
    SparseIpSpace(size_t dim) {
        fstdistfunc_ = SparseIp;
        dim_ = dim;
        data_size_ = sizeof(SparseData);
    }

    size_t get_data_size() override {
        return data_size_;
    }

    hnswlib::DISTFUNC<float> get_dist_func() override {
        return fstdistfunc_;
    }

    void *get_dist_func_param() override {
        return const_cast<size_t*>(&dim_);
    }

    ~SparseIpSpace() {}

    static float SparseIp(const void *pVect1v, const void *pVect2v, const void *dim_ptr);
};

}
}

#endif
