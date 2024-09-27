#include "gtest/gtest.h"
#include "algo/SparseIpSpace.hpp"
#include <vector>
#include <utility>
#include <cmath>
#include <limits>
#include <random>
#include <algorithm>

using namespace hnswlib;
using namespace atinyvectors;
using namespace atinyvectors::algo;

namespace {

// Helper function to compute inner product between two sparse vectors for validation
float ComputeInnerProduct(const SparseData& vec1, const SparseData& vec2) {
    size_t i = 0, j = 0;
    float res = 0.0f;

    while (i < vec1.size() && j < vec2.size()) {
        if (vec1[i].first == vec2[j].first) {
            res += vec1[i].second * vec2[j].second;
            i++;
            j++;
        }
        else if (vec1[i].first < vec2[j].first) {
            i++;
        }
        else {
            j++;
        }
    }

    return res;
}

// Function to generate a random sparse vector
SparseData GenerateRandomSparseData(size_t dim, size_t num_nonzeros) {
    SparseData vec;
    if (num_nonzeros == 0 || dim == 0) {
        return vec; // Return empty vector
    }

    num_nonzeros = std::min(num_nonzeros, dim);

    static std::random_device rd;
    static std::mt19937 rng(rd());

    std::vector<int> indices(dim);
    for (size_t i = 0; i < dim; ++i) {
        indices[i] = static_cast<int>(i);
    }
    std::shuffle(indices.begin(), indices.end(), rng);
    indices.resize(num_nonzeros);
    std::sort(indices.begin(), indices.end());

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (int idx : indices) {
        float value = dist(rng);
        vec.emplace_back(idx, value);
    }

    return vec;
}

}

// Test Fixture for SparseIpSpace
class SparseIpSpaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        dim = 1000; // Example dimension
        space = new SparseIpSpace(dim);
    }

    void TearDown() override {
        delete space;
    }

    size_t dim;
    SpaceInterface<float>* space;
};

// Integration test with HierarchicalNSW
TEST_F(SparseIpSpaceTest, HierarchicalNSWIntegration) {
    HierarchicalNSW<float> index(space, 10000); // Max 10,000 elements

    SparseData vec1 = { {1, 0.5f}, {10, 1.2f}, {500, 3.4f} };
    SparseData vec2 = { {1, 0.6f}, {10, 1.0f}, {700, 2.1f} };
    SparseData vec3 = { {2, 0.4f}, {20, 1.1f}, {600, 2.5f} };

    index.addPoint(&vec1, 0);
    index.addPoint(&vec2, 1);
    index.addPoint(&vec3, 2);

    SparseData query_vec = { {1, 0.55f}, {10, 1.1f}, {500, 3.3f} };

    int k = 2;
    auto result = index.searchKnnCloserFirst(&query_vec, k);

    float ip0 = SparseIpSpace::SparseIp(&query_vec, &vec1, &dim);
    float ip1 = SparseIpSpace::SparseIp(&query_vec, &vec2, &dim);

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].second, 0);
    EXPECT_NEAR(result[0].first, ip0, 1e-4);
    EXPECT_EQ(result[1].second, 1);
    EXPECT_NEAR(result[1].first, ip1, 1e-4);
}

// Random vector generation test
TEST_F(SparseIpSpaceTest, GenerateAndTestRandomSparseDatas) {
    size_t num_vectors = 1000;
    size_t num_nonzeros = 50;

    std::vector<SparseData> random_vectors;
    for (size_t i = 0; i < num_vectors; ++i) {
        random_vectors.push_back(GenerateRandomSparseData(dim, num_nonzeros));
    }

    ASSERT_EQ(random_vectors.size(), num_vectors);
    for (const auto& vec : random_vectors) {
        ASSERT_LE(vec.size(), num_nonzeros);
    }

    SparseIpSpace space(dim);
    HierarchicalNSW<float> index(&space, num_vectors);
    for (size_t i = 0; i < random_vectors.size(); ++i) {
        index.addPoint(&random_vectors[i], i);
    }

    ASSERT_EQ(index.getCurrentElementCount(), num_vectors);

    int k = 5;
    for (size_t test_idx = 0; test_idx < 10; ++test_idx) {
        const auto& query_vec = random_vectors[test_idx];
        auto result = index.searchKnnCloserFirst(&query_vec, k);

        ASSERT_GE(result.size(), 1);
        ASSERT_LE(result.size(), k);

        std::vector<float> distances;
        for (const auto& entry : result) {
            distances.push_back(entry.first);
        }

        EXPECT_TRUE(std::is_sorted(distances.begin(), distances.end()));
    }
}