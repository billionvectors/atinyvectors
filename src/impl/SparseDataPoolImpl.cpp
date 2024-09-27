#include "SparseDataPool.hpp"

namespace atinyvectors
{

SparseDataPool::SparseDataPool() {
    clear();
}

SparseDataPool::~SparseDataPool() {
    clear();
}

SparseData* SparseDataPool::allocate() {
    blocks.emplace_back(std::make_unique<SparseData>());
    return blocks.back().get();
}

void SparseDataPool::deallocate(SparseData* ptr) {
    // do nothing.... it should be cleared when index destroy...
}

void SparseDataPool::clear() {
    std::lock_guard<std::mutex> lock(poolMutex);
    blocks.clear();
}

}