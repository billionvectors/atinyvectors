// tests/test_sparse_l2_space.cpp

#include "gtest/gtest.h"
#include "algo/SparseL2Space.hpp"

#include <vector>
#include <utility>
#include <cmath>
#include <limits>
#include <random>
#include <algorithm>
#include <chrono>

using namespace hnswlib;
using namespace atinyvectors;
using namespace atinyvectors::algo;

namespace {

// Helper function to manually compute L2 distance between two sparse vectors for validation
float ComputeL2Distance(const SparseData& vec1, const SparseData& vec2) {
    size_t i = 0, j = 0;
    float res = 0.0f;

    while (i < vec1.size() && j < vec2.size()) {
        if (vec1[i].first == vec2[j].first) {
            float diff = vec1[i].second - vec2[j].second;
            res += diff * diff;
            i++;
            j++;
        }
        else if (vec1[i].first < vec2[j].first) {
            res += vec1[i].second * vec1[i].second;
            i++;
        }
        else {
            res += vec2[j].second * vec2[j].second;
            j++;
        }
    }

    // Add remaining elements from vec1
    while (i < vec1.size()) {
        res += vec1[i].second * vec1[i].second;
        i++;
    }
    // Add remaining elements from vec2
    while (j < vec2.size()) {
        res += vec2[j].second * vec2[j].second;
        j++;
    }

    return res;
}

// Function to generate a single random sparse vector
SparseData GenerateRandomSparseData(size_t dim, size_t num_nonzeros) {
    SparseData vec;
    if (num_nonzeros == 0 || dim == 0) {
        return vec; // Return empty vector
    }

    // Ensure num_nonzeros does not exceed dim
    num_nonzeros = std::min(num_nonzeros, dim);

    // Initialize a static random number generator for the function
    static std::random_device rd;
    static std::mt19937 rng(rd());
    // For reproducibility, you can use a fixed seed like below:
    // static std::mt19937 rng(42);

    // Generate unique random indices
    std::vector<int> indices(dim);
    for (size_t i = 0; i < dim; ++i) {
        indices[i] = static_cast<int>(i);
    }
    std::shuffle(indices.begin(), indices.end(), rng);
    indices.resize(num_nonzeros);
    std::sort(indices.begin(), indices.end()); // Sort indices for efficient distance computation

    // Generate random float values between 0.0 and 1.0
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (int idx : indices) {
        float value = dist(rng);
        vec.emplace_back(idx, value);
    }

    return vec;
}

// Function to generate multiple random sparse vectors
std::vector<SparseData> GenerateRandomSparseDatas(
    size_t num_vectors,
    size_t dim,
    size_t num_nonzeros_per_vector
) {
    std::vector<SparseData> vectors;
    vectors.reserve(num_vectors);

    // Seed the random number generator with a non-deterministic random device or a fixed seed for reproducibility
    std::random_device rd;
    std::mt19937 rng(rd());
    // For reproducibility, you can use a fixed seed like below:
    // std::mt19937 rng(42);

    for (size_t i = 0; i < num_vectors; ++i) {
        SparseData vec = GenerateRandomSparseData(dim, num_nonzeros_per_vector);
        vectors.emplace_back(std::move(vec));
    }

    return vectors;
}

}

// Test Fixture for SparseL2Space
class SparseL2SpaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        dim = 1000; // Example dimension
        space = new SparseL2Space(dim);
    }

    void TearDown() override {
        delete space;
    }

    size_t dim;
    SpaceInterface<float>* space;
};

TEST_F(SparseL2SpaceTest, HierarchicalNSWIntegration) {
    // Create HierarchicalNSW index
    HierarchicalNSW<float> index(space, 10000); // Max 10,000 elements

    // Create some sparse vectors
    SparseData vec1 = { {1, 0.5f}, {10, 1.2f}, {500, 3.4f} };
    SparseData vec2 = { {1, 0.6f}, {10, 1.0f}, {700, 2.1f} };
    SparseData vec3 = { {2, 0.4f}, {20, 1.1f}, {600, 2.5f} };

    // Add vectors to the index
    index.addPoint(&vec1, 0);
    index.addPoint(&vec2, 1);
    index.addPoint(&vec3, 2);

    // Create a query vector
    SparseData query_vec = { {1, 0.55f}, {10, 1.1f}, {500, 3.3f} };

    // Perform KNN search
    int k = 2;
    auto result = index.searchKnnCloserFirst(&query_vec, k);

    // Extract results
    std::vector<std::pair<float, hnswlib::labeltype>> retrieved;
    for (int i=0;i<k;++i) {
        retrieved.emplace_back(result[i].first, result[i].second);
    }

    // Log the retrieved results
    for (size_t idx = 0; idx < retrieved.size(); ++idx) {
        spdlog::debug("Retrieved {}: label = {}, distance = {}", 
                      idx, retrieved[idx].second, retrieved[idx].first);
    }

    // Since KNN results depend on the internal structure, we validate the distances instead
    float dist0 = SparseL2Space::SparseL2Sqr(&query_vec, &vec1, &dim);
    float dist1 = SparseL2Space::SparseL2Sqr(&query_vec, &vec2, &dim);
    float dist2 = SparseL2Space::SparseL2Sqr(&query_vec, &vec3, &dim);

    spdlog::debug("Computed distances: dist0 = {}, dist1 = {}, dist2 = {}", dist0, dist1, dist2);
    
    // Verify expected top 2 neighbors: vec2 (label 1) and vec1 (label 0)
    EXPECT_EQ(retrieved.size(), 2);

    // Check that the first retrieved label is 1 (vec2) with distance ~15.3125
    EXPECT_EQ(retrieved[0].second, 0);
    EXPECT_NEAR(retrieved[0].first, dist0, 1e-4);

    // Check that the second retrieved label is 0 (vec1) with distance ~0.0225
    EXPECT_EQ(retrieved[1].second, 1);
    EXPECT_NEAR(retrieved[1].first, dist1, 1e-4);
}

TEST_F(SparseL2SpaceTest, GenerateAndTestRandomSparseDatas) {
    size_t num_vectors = 1000;
    size_t dim = 1000; // Dimension of each vector
    size_t num_nonzeros = 50; // Number of non-zero elements per vector

    // Generate random sparse vectors
    std::vector<SparseData> random_vectors = GenerateRandomSparseDatas(num_vectors, dim, num_nonzeros);

    // Verify the number of vectors generated
    ASSERT_EQ(random_vectors.size(), num_vectors);

    // Verify each vector has the correct number of non-zero elements (could be less if dim < num_nonzeros)
    for (const auto& vec : random_vectors) {
        ASSERT_LE(vec.size(), num_nonzeros);
    }

    // Define the space (assuming SparseL2Space is properly defined)
    SparseL2Space space(dim);

    // Create HierarchicalNSW index with the defined space and a maximum of num_vectors elements
    HierarchicalNSW<float> index(&space, num_vectors);

    // Add all random vectors to the HierarchicalNSW index
    for (size_t i = 0; i < random_vectors.size(); ++i) {
        index.addPoint(&random_vectors[i], i);
    }

    // Optionally, ensure all points are added correctly
    ASSERT_EQ(index.getCurrentElementCount(), num_vectors);

    // Perform KNN search for a few sample vectors and verify the results
    int k = 5; // Number of nearest neighbors to retrieve
    for (size_t test_idx = 0; test_idx < 10; ++test_idx) { // Perform 10 random tests
        const auto& query_vec = random_vectors[test_idx];

        // Perform KNN search using HierarchicalNSW
        auto result = index.searchKnnCloserFirst(&query_vec, k);

        // Verify that the number of results is as expected
        ASSERT_GE(result.size(), 1);
        ASSERT_LE(result.size(), k);

        // Extract distances from the search results
        std::vector<float> distances;
        for (const auto& entry : result) {
            distances.emplace_back(entry.first);
        }

        // Check if the distances are sorted in ascending order
        EXPECT_TRUE(std::is_sorted(distances.begin(), distances.end())) 
            << "Test Vector " << test_idx << ": Distances are not sorted in ascending order.";

        // Log the retrieved results for debugging
        for (size_t i = 0; i < result.size(); ++i) {
            spdlog::debug("Test Vector {}: Neighbor {}: label = {}, distance = {}", 
                          test_idx, i, result[i].second, result[i].first);
        }
    }

    // Optionally, perform a KNN search for a vector not in the index to test generality
    SparseData new_query = GenerateRandomSparseData(dim, num_nonzeros);
    auto new_result = index.searchKnnCloserFirst(&new_query, k);

    // Verify the number of results
    ASSERT_LE(new_result.size(), k);

    // Since the query vector is random, just verify that distances are non-negative
    for (const auto& [distance, label] : new_result) {
        EXPECT_GE(distance, 0.0f);
    }

    // Log the retrieved results for the new query
    for (size_t idx = 0; idx < new_result.size(); ++idx) {
        spdlog::debug("New Query Neighbor {}: label = {}, distance = {}", 
                      idx, new_result[idx].second, new_result[idx].first);
    }
}