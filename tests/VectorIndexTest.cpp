#include <gtest/gtest.h>
#include "VectorIndex.hpp"
#include "Version.hpp"
#include "DatabaseManager.hpp"

using namespace atinyvectors;

// Fixture for VectorIndexManager tests
class VectorIndexManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        DatabaseManager::getInstance().reset();
    }

    void TearDown() override {
    }
};

// Test for adding a new VectorIndex
TEST_F(VectorIndexManagerTest, AddVectorIndex) {
    VersionManager& versionManager = VersionManager::getInstance();
    VectorIndexManager& vectorIndexManager = VectorIndexManager::getInstance();

    Version version(0, 1, 0, "Version 1.0", "Initial version", "v1.0", 1627906032, 1627906032);
    versionManager.addVersion(version);

    auto versions = versionManager.getAllVersions();
    ASSERT_FALSE(versions.empty());

    int versionId = versions[0].id;

    // Define HNSW and quantization configurations
    HnswConfig hnswConfig(16, 200);  // Example HNSW config
    QuantizationConfig quantizationConfig;  // Default empty quantization config

    // Add a new vector index
    VectorIndex vectorIndex(0, versionId, VectorValueType::Dense, "Vector Index 1", MetricType::L2, 128,
                            hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
    int vectorIndexId = vectorIndexManager.addVectorIndex(vectorIndex);

    EXPECT_EQ(vectorIndex.id, vectorIndexId);

    auto vectorIndices = vectorIndexManager.getVectorIndicesByVersionId(versionId);
    ASSERT_EQ(vectorIndices.size(), 1);
    EXPECT_EQ(vectorIndices[0].id, vectorIndexId);
    EXPECT_EQ(vectorIndices[0].vectorValueType, VectorValueType::Dense);
}

// Test for getting a vector index by ID
TEST_F(VectorIndexManagerTest, GetVectorIndexById) {
    VersionManager& versionManager = VersionManager::getInstance();
    VectorIndexManager& vectorIndexManager = VectorIndexManager::getInstance();

    Version version(0, 1, 0, "Version 1.0", "Initial version", "v1.0", 1627906032, 1627906032);
    versionManager.addVersion(version);

    auto versions = versionManager.getAllVersions();
    ASSERT_FALSE(versions.empty());

    int versionId = versions[0].id;

    // Define HNSW and quantization configurations
    HnswConfig hnswConfig(16, 200);  // Example HNSW config
    QuantizationConfig quantizationConfig;  // Default empty quantization config

    VectorIndex vectorIndex(0, versionId, VectorValueType::Dense, "Vector Index 1", MetricType::L2, 128,
                            hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
    vectorIndexManager.addVectorIndex(vectorIndex);

    auto vectorIndices = vectorIndexManager.getAllVectorIndices();
    ASSERT_FALSE(vectorIndices.empty());

    int vectorIndexId = vectorIndices[0].id;
    VectorIndex retrievedVectorIndex = vectorIndexManager.getVectorIndexById(vectorIndexId);

    EXPECT_EQ(retrievedVectorIndex.id, vectorIndexId);
    EXPECT_EQ(retrievedVectorIndex.name, "Vector Index 1");
}

// Test for getting vector indices by version ID
TEST_F(VectorIndexManagerTest, GetVectorIndicesByVersionId) {
    VersionManager& versionManager = VersionManager::getInstance();
    VectorIndexManager& vectorIndexManager = VectorIndexManager::getInstance();

    Version version(0, 1, 0, "Version 1.0", "Initial version", "v1.0", 1627906032, 1627906032);
    versionManager.addVersion(version);

    auto versions = versionManager.getAllVersions();
    ASSERT_FALSE(versions.empty());

    int versionId = versions[0].id;

    // Define HNSW and quantization configurations
    HnswConfig hnswConfig(16, 200);  // Example HNSW config
    QuantizationConfig quantizationConfig;  // Default empty quantization config

    // Adding multiple vector indices to the same version
    VectorIndex vectorIndex1(0, versionId, VectorValueType::Dense, "Vector Index 1", MetricType::L2, 128,
                             hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
    VectorIndex vectorIndex2(0, versionId, VectorValueType::Sparse, "Vector Index 2", MetricType::Cosine, 256,
                             hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906033, 1627906033, true);
    vectorIndexManager.addVectorIndex(vectorIndex1);
    vectorIndexManager.addVectorIndex(vectorIndex2);

    auto vectorIndices = vectorIndexManager.getVectorIndicesByVersionId(versionId);
    ASSERT_EQ(vectorIndices.size(), 2);
    EXPECT_EQ(vectorIndices[0].name, "Vector Index 1");
    EXPECT_EQ(vectorIndices[1].name, "Vector Index 2");
}

// Test for updating a vector index
TEST_F(VectorIndexManagerTest, UpdateVectorIndex) {
    VersionManager& versionManager = VersionManager::getInstance();
    VectorIndexManager& vectorIndexManager = VectorIndexManager::getInstance();

    Version version(0, 1, 0, "Version 1.0", "Initial version", "v1.0", 1627906032, 1627906032);
    versionManager.addVersion(version);

    auto versions = versionManager.getAllVersions();
    ASSERT_FALSE(versions.empty());

    int versionId = versions[0].id;

    // Define HNSW and quantization configurations
    HnswConfig hnswConfig(16, 200);  // Example HNSW config
    QuantizationConfig quantizationConfig;  // Default empty quantization config

    VectorIndex vectorIndex(0, versionId, VectorValueType::Dense, "Vector Index 1", MetricType::L2, 128,
                            hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
    vectorIndexManager.addVectorIndex(vectorIndex);

    auto vectorIndices = vectorIndexManager.getAllVectorIndices();
    ASSERT_FALSE(vectorIndices.empty());

    int vectorIndexId = vectorIndices[0].id;
    VectorIndex updatedVectorIndex = vectorIndices[0];
    updatedVectorIndex.name = "Updated Vector Index";
    updatedVectorIndex.vectorValueType = VectorValueType::Combined;

    vectorIndexManager.updateVectorIndex(updatedVectorIndex);

    vectorIndices = vectorIndexManager.getVectorIndicesByVersionId(versionId);
    ASSERT_EQ(vectorIndices.size(), 1);
    EXPECT_EQ(vectorIndices[0].id, vectorIndexId);
    EXPECT_EQ(vectorIndices[0].name, "Updated Vector Index");
    EXPECT_EQ(vectorIndices[0].vectorValueType, VectorValueType::Combined);
}

// Test for deleting a vector index
TEST_F(VectorIndexManagerTest, DeleteVectorIndex) {
    VersionManager& versionManager = VersionManager::getInstance();
    VectorIndexManager& vectorIndexManager = VectorIndexManager::getInstance();

    Version version(0, 1, 0, "Version 1.0", "Initial version", "v1.0", 1627906032, 1627906032);
    versionManager.addVersion(version);

    auto versions = versionManager.getAllVersions();
    ASSERT_FALSE(versions.empty());

    int versionId = versions[0].id;

    // Define HNSW and quantization configurations
    HnswConfig hnswConfig(16, 200);  // Example HNSW config
    QuantizationConfig quantizationConfig;  // Default empty quantization config

    // Add two vector indices
    VectorIndex vectorIndex1(0, versionId, VectorValueType::Dense, "Vector Index 1", MetricType::L2, 128,
                             hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906032, 1627906032, true);
    VectorIndex vectorIndex2(0, versionId, VectorValueType::Sparse, "Vector Index 2", MetricType::Cosine, 256,
                             hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 1627906033, 1627906033, true);
    vectorIndexManager.addVectorIndex(vectorIndex1);
    vectorIndexManager.addVectorIndex(vectorIndex2);

    // Delete the second vector index
    vectorIndexManager.deleteVectorIndex(vectorIndex2.id);

    auto vectorIndices = vectorIndexManager.getVectorIndicesByVersionId(versionId);
    ASSERT_EQ(vectorIndices.size(), 1);
    EXPECT_EQ(vectorIndices[0].id, vectorIndex1.id);
}

// Test for handling a non-existent vector index
TEST_F(VectorIndexManagerTest, HandleNonExistentVectorIndex) {
    VectorIndexManager& manager = VectorIndexManager::getInstance();

    // Trying to get a vector index with an ID that does not exist
    EXPECT_THROW(manager.getVectorIndexById(999), std::runtime_error);
}
