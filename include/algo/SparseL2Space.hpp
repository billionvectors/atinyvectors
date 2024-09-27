#ifndef __ATINYVECTORS_SPARSE_L2SPACE_HPP__
#define __ATINYVECTORS_SPARSE_L2SPACE_HPP__

#include "hnswlib/hnswlib.h"
#include "spdlog/spdlog.h"
#include <vector>
#include <algorithm>
#include <cmath>

#include "ValueType.hpp"

namespace atinyvectors
{
namespace algo
{

class SparseL2Space : public hnswlib::SpaceInterface<float> {
    hnswlib::DISTFUNC<float> fstdistfunc_;
    size_t data_size_;
    size_t dim_;

public:
    SparseL2Space(size_t dim) {
        fstdistfunc_ = SparseL2Sqr;
        dim_ = dim;
        data_size_ = sizeof(SparseData); // it can not be fixed size
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

    ~SparseL2Space() {}

    static float SparseL2Sqr(const void *pVect1v, const void *pVect2v, const void *dim_ptr);
};

}
}

#endif