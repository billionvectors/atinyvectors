// SparseDataPool.hpp
#ifndef __ATINYVECTORS_SPARSEDATAPOOL_HPP__
#define __ATINYVECTORS_SPARSEDATAPOOL_HPP__

#include <vector>
#include <utility>
#include <stack>
#include <mutex>
#include <memory>
#include "ValueType.hpp"

namespace atinyvectors {

class SparseDataPool {
public:
    SparseDataPool();
    ~SparseDataPool();

    SparseData* allocate();
    void deallocate(SparseData* ptr);
    void clear();

private:
    SparseDataPool(const SparseDataPool&) = delete;
    SparseDataPool& operator=(const SparseDataPool&) = delete;

    friend struct std::default_delete<SparseDataPool>;

    std::vector<std::unique_ptr<SparseData>> blocks;
    std::mutex poolMutex;
};
}

#endif // SPARSEDATAPOOL_HPP
