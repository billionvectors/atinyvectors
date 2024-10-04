// SearchDTOTest.cpp

#include <gtest/gtest.h>
#include "dto/SpaceDTO.hpp"
#include "dto/SearchDTO.hpp"
#include "dto/VectorDTO.hpp"
#include "algo/HnswIndexLRUCache.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "Snapshot.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "RbacToken.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include <vector>
#include <cmath>

using namespace atinyvectors;
using namespace atinyvectors::dto;
using namespace atinyvectors::algo;
using namespace nlohmann;

// Test Fixture class
class SearchDTOTest : public ::testing::Test {
protected:
    void SetUp() override {
        IdCache::getInstance().clean();

        SnapshotManager& snapshotManager = SnapshotManager::getInstance();
        snapshotManager.createTable();

        SpaceManager& spaceManager = SpaceManager::getInstance();
        spaceManager.createTable();
        
        VectorIndexManager& vectorIndexManager = VectorIndexManager::getInstance();
        vectorIndexManager.createTable();

        VersionManager& versionManager = VersionManager::getInstance();
        versionManager.createTable();

        VectorMetadataManager& metadataManager = VectorMetadataManager::getInstance();
        metadataManager.createTable();

        VectorManager& vectorManager = VectorManager::getInstance();
        vectorManager.createTable();

        RbacTokenManager& rbacTokenManager = RbacTokenManager::getInstance();
        rbacTokenManager.createTable();

        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM RbacToken;");

        // clean data
        HnswIndexLRUCache::getInstance().clean();
    }

    void TearDown() override {
        // Clear data after each test
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
        db.exec("DELETE FROM RbacToken;");
    }
};

TEST_F(SearchDTOTest, VectorSearchWithJSONQuery) {
    // Set up Space, Version, and VectorIndex for search
    Space defaultSpace(0, "VectorSearchWithJSONQuery", "Default Space Description", 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(defaultSpace);

    // Use versionUniqueId as 1 for the test case
    Version defaultVersion(0, spaceId, 1, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    // Cache the versionId using IdCache
    IdCache::getInstance().getVersionId("VectorSearchWithJSONQuery", 1); // Assuming this populates the cache

    // Create default HNSW and Quantization configurations
    HnswConfig hnswConfig(16, 200);  // Example HNSW config: M = 16, EfConstruct = 200
    QuantizationConfig quantizationConfig;  // Default empty quantization config

    // Create and add a default VectorIndex
    VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", MetricType::L2, 4,
                             hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 0, 0, true);
    int indexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);

    // Insert some vectors for search
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"}
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"}
            }
        ]
    })";

    VectorDTOManager vectorDTOManager;
    vectorDTOManager.upsert("VectorSearchWithJSONQuery", 1, vectorDataJson);

    // Prepare search query JSON
    std::string queryJsonStr = R"({
        "vector": [0.25, 0.45, 0.75, 0.85]
    })";

    // Perform the search using SearchDTOManager
    SearchDTOManager searchManager;
    auto searchResults = searchManager.search("VectorSearchWithJSONQuery", 1, queryJsonStr, 2);

    // Log distances and result metadata for debugging
    spdlog::info("Search results size: {}", searchResults.size());
    for (const auto& result : searchResults) {
        spdlog::info("Distance: {}, ID: {}", result.first, result.second);
    }

    // Validate that two results are returned
    ASSERT_EQ(searchResults.size(), 2);  

    // Validate distances
    double distance1 = searchResults[0].first;
    double distance2 = searchResults[1].first;

    // Log distances for debugging
    spdlog::info("Distance1: {}", distance1);
    spdlog::info("Distance2: {}", distance2);

    // Expected distances
    double expectedDistance1 = 0.0; // Distance is 0 for the same vector
    double expectedDistance2 = (
        std::pow(0.25 - 0.20, 2) + std::pow(0.45 - 0.62, 2) +
        std::pow(0.75 - 0.77, 2) + std::pow(0.85 - 0.75, 2)
    ); // Euclidean distance to the second vector

    EXPECT_NEAR(distance1, expectedDistance1, 1e-6); // Compare within allowable error range
    EXPECT_NEAR(distance2, expectedDistance2, 1e-6);

    // Check if metadata is intact for the found vectors
    auto vector1 = VectorManager::getInstance().getVectorByUniqueId(versionId, 1);
    auto metadataList1 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector1.id);
    spdlog::info("Vector1 metadata: key = {}, value = {}", metadataList1[0].key, metadataList1[0].value);
    EXPECT_EQ(metadataList1[0].key, "category");
    EXPECT_EQ(metadataList1[0].value, "A");

    auto vector2 = VectorManager::getInstance().getVectorByUniqueId(versionId, 2);
    auto metadataList2 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector2.id);
    spdlog::info("Vector2 metadata: key = {}, value = {}", metadataList2[0].key, metadataList2[0].value);
    EXPECT_EQ(metadataList2[0].key, "category");
    EXPECT_EQ(metadataList2[0].value, "B");
}

TEST_F(SearchDTOTest, VectorSearchWithSparseVectors) {
    // Set up Space, Version, and VectorIndex for sparse search
    Space sparseSpace(0, "VectorSearchWithSparseVectors", "Space for Sparse Vector Search", 0, 0);
    int sparseSpaceId = SpaceManager::getInstance().addSpace(sparseSpace);

    // Use versionUniqueId as 2 for the sparse test case
    Version sparseVersion(0, sparseSpaceId, 1, "Sparse Version", "Version for sparse vectors", "v2", 0, 0, true);
    int sparseVersionId = VersionManager::getInstance().addVersion(sparseVersion);

    // Create default HNSW and Quantization configurations for sparse vectors
    HnswConfig hnswConfigSparse(16, 200);  // Example HNSW config: M = 16, EfConstruct = 200
    QuantizationConfig quantizationConfigSparse;  // Default empty quantization config

    // Create and add a VectorIndex for Sparse vectors
    VectorIndex sparseIndex(0, sparseVersionId, VectorValueType::Sparse, "Sparse Index", MetricType::L2, 4,
                            hnswConfigSparse.toJson().dump(), quantizationConfigSparse.toJson().dump(), 0, 0, true);
    int sparseIndexId = VectorIndexManager::getInstance().addVectorIndex(sparseIndex);

    // Insert some sparse vectors for search
    std::string sparseVectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "sparse_data": {
                    "indices": [0, 2],
                    "values": [0.5, 0.8]
                },
                "metadata": {"category": "C"}
            },
            {
                "id": 2,
                "sparse_data": {
                    "indices": [1, 3],
                    "values": [0.9, 1.0]
                },
                "metadata": {"category": "D"}
            }
        ]
    })";

    VectorDTOManager vectorDTOManagerSparse;
    vectorDTOManagerSparse.upsert("VectorSearchWithSparseVectors", 1, sparseVectorDataJson);
    spdlog::debug("Upsert success! prepare search");

    // Prepare search query JSON for sparse vector
    std::string sparseQueryJsonStr = R"({
        "sparse_data": {
            "indices": [0, 2],
            "values": [0.5, 0.8]
        }
    })";

    // Perform the search using SearchDTOManager
    SearchDTOManager searchManagerSparse;
    auto searchResultsSparse = searchManagerSparse.search("VectorSearchWithSparseVectors", 1, sparseQueryJsonStr, 2);

    // Log distances and result metadata for debugging
    spdlog::info("Sparse Search results size: {}", searchResultsSparse.size());
    for (const auto& result : searchResultsSparse) {
        spdlog::info("Distance: {}, ID: {}", result.first, result.second);
    }

    // Validate that two results are returned
    ASSERT_EQ(searchResultsSparse.size(), 2);  

    // Validate distances
    double distanceSparse1 = searchResultsSparse[0].first;
    double distanceSparse2 = searchResultsSparse[1].first;

    // Log distances for debugging
    spdlog::info("Sparse Distance1: {}", distanceSparse1);
    spdlog::info("Sparse Distance2: {}", distanceSparse2);

    // Expected distances
    double expectedDistanceSparse1 = 0.0; // Distance is 0 for the same vector
    double expectedDistanceSparse2 = (
        std::pow(0.5, 2) + std::pow(0.9, 2) + std::pow(0.8, 2) + std::pow(1.0, 2)
    ); // Euclidean distance to the second sparse vector

    EXPECT_NEAR(distanceSparse1, expectedDistanceSparse1, 1e-6); // Compare within allowable error range
    EXPECT_NEAR(distanceSparse2, expectedDistanceSparse2, 1e-6);

    // Check if metadata is intact for the found vectors
    auto sparseVector1 = VectorManager::getInstance().getVectorByUniqueId(sparseVersionId, 1);
    auto sparseMetadataList1 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(sparseVector1.id);
    spdlog::info("Sparse Vector1 metadata: key = {}, value = {}", sparseMetadataList1[0].key, sparseMetadataList1[0].value);
    EXPECT_EQ(sparseMetadataList1[0].key, "category");
    EXPECT_EQ(sparseMetadataList1[0].value, "C");

    auto sparseVector2 = VectorManager::getInstance().getVectorByUniqueId(sparseVersionId, 2);
    auto sparseMetadataList2 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(sparseVector2.id);
    spdlog::info("Sparse Vector2 metadata: key = {}, value = {}", sparseMetadataList2[0].key, sparseMetadataList2[0].value);
    EXPECT_EQ(sparseMetadataList2[0].key, "category");
    EXPECT_EQ(sparseMetadataList2[0].value, "D");
}

TEST_F(SearchDTOTest, VectorSearchWithJSONQueryUsingDefaultVersion) {
    // Create a space
    std::string inputJson = R"({
        "name": "VectorSearchWithDefaultVersion",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceDTOManager spaceDTOManager;
    spaceDTOManager.createSpace(inputJson);

    // Retrieve the spaceId
    Space space = SpaceManager::getInstance().getSpaceByName("VectorSearchWithDefaultVersion");
    ASSERT_NE(space.id, 0); // Ensure space was created

    // Retrieve the versionId associated with the space using IdCache
    int versionId = IdCache::getInstance().getVersionId("VectorSearchWithDefaultVersion", 0); // unique_id is 0 for default version

    // Create default HNSW and Quantization configurations
    HnswConfig hnswConfig(16, 200);  // Example HNSW config: M = 16, EfConstruct = 200
    QuantizationConfig quantizationConfig;  // Default empty quantization config

    // Create and add a default VectorIndex
    VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", MetricType::L2, 4,
                             hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 0, 0, true);
    int indexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);

    // Insert some vectors for search
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"}
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"}
            }
        ]
    })";

    VectorDTOManager vectorDTOManager;
    vectorDTOManager.upsert("VectorSearchWithDefaultVersion", 0, vectorDataJson); // versionUniqueId is 0

    // Prepare search query JSON
    std::string queryJsonStr = R"({
        "vector": [0.25, 0.45, 0.75, 0.85]
    })";

    // Perform the search using SearchDTOManager
    SearchDTOManager searchManager;
    auto searchResults = searchManager.search("VectorSearchWithDefaultVersion", 0, queryJsonStr, 2); // default versionUniqueId is 0

    // Log distances and result metadata for debugging
    spdlog::info("Search results size: {}", searchResults.size());
    for (const auto& result : searchResults) {
        spdlog::info("Distance: {}, ID: {}", result.first, result.second);
    }

    // Validate that two results are returned
    ASSERT_EQ(searchResults.size(), 2);

    // Validate distances
    double distance1 = searchResults[0].first;
    double distance2 = searchResults[1].first;

    // Log distances for debugging
    spdlog::info("Distance1: {}", distance1);
    spdlog::info("Distance2: {}", distance2);

    // Expected distances
    double expectedDistance1 = 0.0; // Distance is 0 for the same vector
    double expectedDistance2 = (
        std::pow(0.25 - 0.20, 2) + std::pow(0.45 - 0.62, 2) +
        std::pow(0.75 - 0.77, 2) + std::pow(0.85 - 0.75, 2)
    ); // Euclidean distance to the second vector

    EXPECT_NEAR(distance1, expectedDistance1, 1e-6); // Compare within allowable error range
    EXPECT_NEAR(distance2, expectedDistance2, 1e-6);

    // Check if metadata is intact for the found vectors
    auto vector1 = VectorManager::getInstance().getVectorByUniqueId(versionId, 1);
    auto metadataList1 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector1.id);
    spdlog::info("Vector1 metadata: key = {}, value = {}", metadataList1[0].key, metadataList1[0].value);
    EXPECT_EQ(metadataList1[0].key, "category");
    EXPECT_EQ(metadataList1[0].value, "A");

    auto vector2 = VectorManager::getInstance().getVectorByUniqueId(versionId, 2);
    auto metadataList2 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector2.id);
    spdlog::info("Vector2 metadata: key = {}, value = {}", metadataList2[0].key, metadataList2[0].value);
    EXPECT_EQ(metadataList2[0].key, "category");
    EXPECT_EQ(metadataList2[0].value, "B");
}

TEST_F(SearchDTOTest, VectorSearchWithCosineMetric) {
    // Set up Space, Version, and VectorIndex for Cosine search
    Space cosineSpace(0, "VectorSearchWithCosineMetric", "Space for Cosine metric", 0, 0);
    int cosineSpaceId = SpaceManager::getInstance().addSpace(cosineSpace);

    // Use versionUniqueId as 1 for the Cosine test case
    Version cosineVersion(0, cosineSpaceId, 1, "Cosine Version", "Version for Cosine metric", "v1", 0, 0, true);
    int cosineVersionId = VersionManager::getInstance().addVersion(cosineVersion);

    // Cache the versionId using IdCache
    IdCache::getInstance().getVersionId("VectorSearchWithCosineMetric", 1); // Assuming this populates the cache

    // Create HNSW and Quantization configs for Cosine metric
    HnswConfig hnswConfigCosine(16, 200);  // Example HNSW config: M = 16, EfConstruct = 200
    QuantizationConfig quantizationConfigCosine;  // Default empty quantization config

    // Create and add a VectorIndex with Cosine metric and Dense vectors
    VectorIndex cosineIndex(0, cosineVersionId, VectorValueType::Dense, "Cosine Index", MetricType::Cosine, 4,
                            hnswConfigCosine.toJson().dump(), quantizationConfigCosine.toJson().dump(), 0, 0, true);
    int cosineIndexId = VectorIndexManager::getInstance().addVectorIndex(cosineIndex);

    // Insert 5 vectors for Cosine search
    std::string vectorDataCosineJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [1.0, 0.0, 0.0, 0.0],
                "metadata": {"category": "X"}
            },
            {
                "id": 2,
                "data": [0.0, 1.0, 0.0, 0.0],
                "metadata": {"category": "Y"}
            },
            {
                "id": 3,
                "data": [-1.0, 0.0, 0.0, 0.0],
                "metadata": {"category": "Z"}
            },
            {
                "id": 4,
                "data": [1.0, 1.0, 0.0, 0.0],
                "metadata": {"category": "W"}
            },
            {
                "id": 5,
                "data": [1.0, 0.0, 1.0, 0.0],
                "metadata": {"category": "V"}
            }
        ]
    })";

    VectorDTOManager vectorDTOManagerCosine;
    vectorDTOManagerCosine.upsert("VectorSearchWithCosineMetric", 1, vectorDataCosineJson);

    // Prepare search query JSON for Cosine metric
    std::string queryCosineJsonStr = R"({
        "vector": [1.0, 0.0, 0.0, 0.0]
    })";

    // Perform the search using SearchDTOManager
    SearchDTOManager searchManagerCosine;
    auto searchResultsCosine = searchManagerCosine.search("VectorSearchWithCosineMetric", 1, queryCosineJsonStr, 5);

    // Log distances and result metadata for debugging
    spdlog::info("Cosine Search results size: {}", searchResultsCosine.size());
    for (const auto& result : searchResultsCosine) {
        spdlog::info("Distance (1 - Cosine Similarity): {}, ID: {}", result.first, result.second);
    }

    // Validate that five results are returned
    ASSERT_EQ(searchResultsCosine.size(), 5);

    // Define expected distances based on 1 - Cosine Similarity
    // Vector 1: same as query -> 0.0
    // Vector 2: orthogonal -> 1.0
    // Vector 3: opposite -> 2.0
    // Vector 4: 45 degrees -> 1 - (cos(45°)) = 1 - (1/sqrt(2)) ≈ 0.292893
    // Vector 5: 45 degrees in another plane -> 1 - (cos(45°)) = 1 - (1/sqrt(2)) ≈ 0.292893

    // Create a map from ID to expected distance
    std::map<int, double> expectedDistances = {
        {1, 0.0},
        {2, 1.0},
        {3, 2.0},
        {4, 1.0 - (1.0 / std::sqrt(2.0))}, // ≈0.292893
        {5, 1.0 - (1.0 / std::sqrt(2.0))}  // ≈0.292893
    };

    // Iterate through search results and validate
    for (const auto& result : searchResultsCosine) {
        float distance = result.first;
        int id = result.second;

        // Log each result
        spdlog::info("Vector ID: {}, Distance (1 - Cosine Similarity): {}", id, distance);

        // Check if the ID exists in expectedDistances
        ASSERT_TRUE(expectedDistances.find(id) != expectedDistances.end()) << "Unexpected vector ID found: " << id;

        double expectedDistance = expectedDistances[id];
        EXPECT_NEAR(distance, expectedDistance, 1e-6) << "Mismatch in distance for vector ID: " << id;
    }

    // Check if metadata is intact for the found vectors
    for (const auto& [id, expectedDistance] : expectedDistances) {
        auto vector = VectorManager::getInstance().getVectorByUniqueId(cosineVersionId, id);
        auto metadataList = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector.id);
        spdlog::info("Vector ID: {}, Metadata: key = {}, value = {}", id, metadataList[0].key, metadataList[0].value);
        EXPECT_EQ(metadataList[0].key, "category");
        // Validate metadata value based on vector ID
        if (id == 1) {
            EXPECT_EQ(metadataList[0].value, "X");
        } else if (id == 2) {
            EXPECT_EQ(metadataList[0].value, "Y");
        } else if (id == 3) {
            EXPECT_EQ(metadataList[0].value, "Z");
        } else if (id == 4) {
            EXPECT_EQ(metadataList[0].value, "W");
        } else if (id == 5) {
            EXPECT_EQ(metadataList[0].value, "V");
        }
    }
}