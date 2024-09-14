#include <gtest/gtest.h>
#include "VectorIndexOptimizer.hpp"
#include "DatabaseManager.hpp"

using namespace atinyvectors;

// Fixture for VectorIndexOptimizerManager tests
class VectorIndexOptimizerManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up a clean test database
        VectorIndexOptimizerManager& optimizerManager = VectorIndexOptimizerManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM VectorIndexOptimizer;");
    }

    void TearDown() override {
        // Cleanup after each test if necessary
        VectorIndexOptimizerManager& optimizerManager = VectorIndexOptimizerManager::getInstance();
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM VectorIndexOptimizer;");
    }
};

// Test for adding a new VectorIndexOptimizer
TEST_F(VectorIndexOptimizerManagerTest, AddOptimizer) {
    VectorIndexOptimizerManager& optimizerManager = VectorIndexOptimizerManager::getInstance();

    VectorIndexOptimizer optimizer(0, 1, MetricType::L2, 128);
    int optimizerId = optimizerManager.addOptimizer(optimizer);

    EXPECT_EQ(optimizer.id, optimizerId);

    auto optimizers = optimizerManager.getAllOptimizers();
    ASSERT_EQ(optimizers.size(), 1);
    EXPECT_EQ(optimizers[0].id, optimizerId);
}

// Test for getting an optimizer by ID
TEST_F(VectorIndexOptimizerManagerTest, GetOptimizerById) {
    VectorIndexOptimizerManager& optimizerManager = VectorIndexOptimizerManager::getInstance();

    VectorIndexOptimizer optimizer(0, 1, MetricType::L2, 128);
    optimizerManager.addOptimizer(optimizer);

    auto optimizers = optimizerManager.getAllOptimizers();
    ASSERT_FALSE(optimizers.empty());

    int optimizerId = optimizers[0].id;
    VectorIndexOptimizer retrievedOptimizer = optimizerManager.getOptimizerById(optimizerId);

    EXPECT_EQ(retrievedOptimizer.id, optimizerId);
}

// Test for updating an optimizer
TEST_F(VectorIndexOptimizerManagerTest, UpdateOptimizer) {
    VectorIndexOptimizerManager& optimizerManager = VectorIndexOptimizerManager::getInstance();

    VectorIndexOptimizer optimizer(0, 1, MetricType::L2, 128);
    optimizerManager.addOptimizer(optimizer);

    auto optimizers = optimizerManager.getAllOptimizers();
    ASSERT_FALSE(optimizers.empty());

    int optimizerId = optimizers[0].id;
    VectorIndexOptimizer updatedOptimizer = optimizers[0];
    updatedOptimizer.dimension = 256;

    optimizerManager.updateOptimizer(updatedOptimizer);

    optimizers = optimizerManager.getAllOptimizers();
    ASSERT_EQ(optimizers.size(), 1);
    EXPECT_EQ(optimizers[0].id, optimizerId);
    EXPECT_EQ(optimizers[0].dimension, 256);
}

// Test for deleting an optimizer
TEST_F(VectorIndexOptimizerManagerTest, DeleteOptimizer) {
    VectorIndexOptimizerManager& optimizerManager = VectorIndexOptimizerManager::getInstance();

    // Add two optimizers
    VectorIndexOptimizer optimizer1(0, 1, MetricType::L2, 128);
    VectorIndexOptimizer optimizer2(0, 2, MetricType::InnerProduct, 256);
    optimizerManager.addOptimizer(optimizer1);
    optimizerManager.addOptimizer(optimizer2);

    // Delete the second optimizer
    optimizerManager.deleteOptimizer(optimizer2.id);

    auto optimizers = optimizerManager.getAllOptimizers();
    ASSERT_EQ(optimizers.size(), 1);
    EXPECT_EQ(optimizers[0].id, optimizer1.id);
}

// Test for handling a non-existent optimizer
TEST_F(VectorIndexOptimizerManagerTest, HandleNonExistentOptimizer) {
    VectorIndexOptimizerManager& manager = VectorIndexOptimizerManager::getInstance();

    // Trying to get an optimizer with an ID that does not exist
    EXPECT_THROW(manager.getOptimizerById(999), std::runtime_error);
}

