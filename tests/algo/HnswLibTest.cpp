#include <iostream>
#include <vector>
#include <random>
#include "hnswlib/hnswlib.h"
#include "gtest/gtest.h"

class HnswlibTest : public ::testing::Test {
protected:
    void SetUp() override {
        dim = 16;
        num_elements = 100;

        l2space = new hnswlib::L2Space(dim);
        index = new hnswlib::HierarchicalNSW<float>(l2space, num_elements);

        // Initialize random number generator for generating random float values
        std::random_device rd;  // Random number generator
        std::mt19937 gen(rd()); // Seed with a real random value, if available
        std::uniform_real_distribution<> dis(0.0, 1.0); // Uniform distribution in the range [0, 1]

        // Add elements to the index with random data
        for (size_t i = 0; i < num_elements; i++) {
            std::vector<float> data(dim);
            for (int j = 0; j < dim; j++) {
                data[j] = static_cast<float>(dis(gen));  // Generate random float value
            }
            index->addPoint(data.data(), i);

            // Cache generated data for later testing
            cached_data.push_back(data);
        }

        // Cache the query vectors based on random data
        cached_query_5 = cached_data[5];   // Using the 6th element as a query
        cached_query_10 = cached_data[10]; // Using the 11th element as a query
    }

    void TearDown() override {
        // Cleanup
        delete index;
        delete l2space;
    }

    size_t dim;
    size_t num_elements;
    hnswlib::L2Space* l2space;
    hnswlib::HierarchicalNSW<float>* index;

    // Cached query vectors
    std::vector<float> cached_query_5;
    std::vector<float> cached_query_10;

    // Cache data for testing
    std::vector<std::vector<float>> cached_data;
};

TEST_F(HnswlibTest, TestAddAndSearch) {
    size_t k = 5;
    auto result = index->searchKnnCloserFirst(cached_query_5.data(), k);
    EXPECT_EQ(result[0].second, 5);
}

TEST_F(HnswlibTest, TestAddAndSearchMultiple) {
    size_t k = 3;
    auto result = index->searchKnnCloserFirst(cached_query_10.data(), k);
    EXPECT_EQ(result[0].second, 10);
}

TEST_F(HnswlibTest, TestSaveAndLoadIndex) {
    const std::string index_path = "test_hnsw_index.bin";

    index->saveIndex(index_path);

    hnswlib::HierarchicalNSW<float> new_index(l2space, index_path);

    size_t k = 5;

    auto result = new_index.searchKnnCloserFirst(cached_query_5.data(), k);
    EXPECT_EQ(result[0].second, 5);

    std::remove(index_path.c_str());
}
