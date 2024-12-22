// VectorServiceTest.cpp

#include <gtest/gtest.h>
#include "service/SpaceService.hpp"
#include "service/VectorService.hpp"
#include "algo/FaissIndexLRUCache.hpp"
#include "utils/Utils.hpp"
#include "BM25.hpp"
#include "Snapshot.hpp"
#include "Space.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "Version.hpp"
#include "VectorMetadata.hpp"
#include "DatabaseManager.hpp"
#include "IdCache.hpp"
#include "RbacToken.hpp"
#include "nlohmann/json.hpp"

using namespace atinyvectors;
using namespace atinyvectors::service;
using namespace atinyvectors::algo;
using namespace atinyvectors::utils;
using namespace nlohmann;

// Test Fixture class
class VectorServiceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        IdCache::getInstance().clean();

        // clean data
        DatabaseManager::getInstance().reset();
        FaissIndexLRUCache::getInstance().clean();
    }

    void TearDown() override {
    }
};

TEST_F(VectorServiceManagerTest, UpsertVectorsWithJson) {
    // Set up necessary default Space and Version
    Space defaultSpace(0, "default_space", "Default Space Description", 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(defaultSpace);

    Version defaultVersion(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    // Create a default VectorIndex if none exists
    auto vectorIndices = VectorIndexManager::getInstance().getVectorIndicesByVersionId(versionId);
    int defaultIndexId = 0;
    if (vectorIndices.empty()) {
        // Use updated constructor to create VectorIndex
        HnswConfig hnswConfig(16, 200);  // Example HNSW config: M = 16, EfConstruct = 200
        QuantizationConfig quantizationConfig;  // Default empty quantization config
        
        VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", MetricType::L2, 128,
                                 hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 0, 0, true);
        defaultIndexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);
    } else {
        defaultIndexId = vectorIndices[0].id;
    }

    VectorServiceManager manager;

    // Prepare JSON with both Dense and Sparse vectors
    std::string updateJson = R"({
        "vectors": [
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"city": "New York"}
            },
            {
                "id": 3,
                "sparse_data": {
                    "indices": [1, 3],
                    "values": [0.9, 1.0]
                },
                "metadata": {"city": "Los Angeles"}
            }
        ]
    })";

    // Perform upsert operation
    manager.upsert("default_space", 0, updateJson);

    // Fetch and verify Dense Vector with unique_id 2
    auto vector1 = VectorManager::getInstance().getVectorByUniqueId(versionId, 2);
    ASSERT_EQ(vector1.unique_id, 2);  // Check unique_id
    ASSERT_EQ(vector1.values.size(), 1);  // Should have one VectorValue
    EXPECT_EQ(vector1.values[0].type, VectorValueType::Dense);
    EXPECT_EQ(vector1.values[0].denseData, std::vector<float>({0.20f, 0.62f, 0.77f, 0.75f}));

    // Check metadata for Dense Vector
    auto metadataList1 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector1.id);
    ASSERT_EQ(metadataList1.size(), 1); // Only one metadata entry should exist
    EXPECT_EQ(metadataList1[0].key, "city");
    EXPECT_EQ(metadataList1[0].value, "New York");

    // Fetch and verify Sparse Vector with unique_id 3
    auto vector2 = VectorManager::getInstance().getVectorByUniqueId(versionId, 3);
    ASSERT_EQ(vector2.unique_id, 3);  // Check unique_id
    ASSERT_EQ(vector2.values.size(), 1);  // Should have one VectorValue
    EXPECT_EQ(vector2.values[0].type, VectorValueType::Sparse);
    
    SparseData testData = { {1, 0.9}, {3, 1.0f}};
    EXPECT_EQ(*vector2.values[0].sparseData, testData);

    // Check metadata for Sparse Vector
    auto metadataList2 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector2.id);
    ASSERT_EQ(metadataList2.size(), 1); // Only one metadata entry should exist
    EXPECT_EQ(metadataList2[0].key, "city");
    EXPECT_EQ(metadataList2[0].value, "Los Angeles");
}

TEST_F(VectorServiceManagerTest, UpsertStandaloneDenseAndSparseVectors) {
    // Set up necessary default Space and Version
    Space defaultSpace(0, "default_space", "Default Space Description", 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(defaultSpace);

    Version defaultVersion(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    // Create a default VectorIndex if none exists
    auto vectorIndices = VectorIndexManager::getInstance().getVectorIndicesByVersionId(versionId);
    int defaultIndexId = 0;
    if (vectorIndices.empty()) {
        // Use updated constructor to create VectorIndex
        HnswConfig hnswConfig(16, 200);  // Example HNSW config: M = 16, EfConstruct = 200
        QuantizationConfig quantizationConfig;  // Default empty quantization config
        
        VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", MetricType::L2, 128,
                                 hnswConfig.toJson().dump(), quantizationConfig.toJson().dump(), 0, 0, true);
        defaultIndexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);
    } else {
        defaultIndexId = vectorIndices[0].id;
    }

    VectorServiceManager manager;

    // Prepare JSON with standalone Dense and Sparse vectors
    std::string updateJson = R"({
        "data": [
            {
                "data": [0.10, 0.20, 0.30, 0.40],
                "metadata": {"category": "A"}
            },
            {
                "indices": [0, 2],
                "values": [0.5, 0.7],
                "metadata": {"category": "B"}
            }
        ]
    })";

    // Perform upsert operation
    manager.upsert("default_space", 0, updateJson);

    // Fetch all vectors by versionId
    json fetchedJson = manager.getVectorsByVersionId("default_space", 1, 0, 10);
    ASSERT_TRUE(fetchedJson.contains("vectors"));
    ASSERT_TRUE(fetchedJson["vectors"].is_array());
    ASSERT_EQ(fetchedJson["vectors"].size(), 2);

    // Iterate through fetched vectors and verify
    for (const auto& vectorJson : fetchedJson["vectors"]) {
        int unique_id = vectorJson["id"].get<int>();
        if (unique_id == 0) {
            // Since standalone vectors have unique_id = 0, we need another way to identify them.
            // Alternatively, modify the insertion to assign unique_ids if necessary.
            // For this test, we'll assume they have been assigned unique_ids sequentially.
            // Fetch vectors by their data.
            const auto& data = vectorJson["data"];
            if (data.contains("data")) {
                // Dense Vector
                EXPECT_EQ(data["data"], std::vector<float>({0.10f, 0.20f, 0.30f, 0.40f}));

                // Check metadata
                ASSERT_TRUE(vectorJson.contains("metadata"));
                EXPECT_EQ(vectorJson["metadata"]["category"], "A");
            } else if (data.contains("sparse_data")) {
                // Sparse Vector
                EXPECT_EQ(data["sparse_data"]["indices"], std::vector<int>({0, 2}));
                EXPECT_EQ(data["sparse_data"]["values"], std::vector<float>({0.5f, 0.7f}));

                // Check metadata
                ASSERT_TRUE(vectorJson.contains("metadata"));
                EXPECT_EQ(vectorJson["metadata"]["category"], "B");
            } else {
                FAIL() << "Vector data does not contain expected fields.";
            }
        }
    }
}

TEST_F(VectorServiceManagerTest, UpsertVectorsWithDoc) {
    // Create a space
    std::string inputJson = R"({
        "name": "UpsertVectorsWithDoc",
        "dimension": 4,
        "metric": "L2",
        "hnsw_config": {
            "M": 16,
            "ef_construct": 100
        }
    })";

    SpaceServiceManager spaceServiceManager;
    spaceServiceManager.createSpace(inputJson);

    VectorServiceManager manager;

    // Prepare JSON with Dense vector containing doc and doc_tokens
    std::string vectorDataJson = R"({
        "vectors": [
            {
                "id": 1,
                "data": [0.25, 0.45, 0.75, 0.85],
                "metadata": {"category": "A"},
                "doc": "This is a test document for vector 1",
                "doc_tokens": ["test", "document", "vector", "1"]
            },
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"category": "B"},
                "doc": "Another test document for vector 2",
                "doc_tokens": ["another", "test", "document", "vector", "2"]
            }
        ]
    })";

    // Perform upsert operation
    manager.upsert("UpsertVectorsWithDoc", 0, vectorDataJson);

    // Verify BM25 entries for the vectors
    auto bm25Doc1 = BM25Manager::getInstance().getDocByVectorId(1);
    ASSERT_EQ(bm25Doc1, "This is a test document for vector 1");

    auto bm25Doc2 = BM25Manager::getInstance().getDocByVectorId(2);
    ASSERT_EQ(bm25Doc2, "Another test document for vector 2");

    // Fetch and verify vectors by versionId
    json fetchedVectors = manager.getVectorsByVersionId("UpsertVectorsWithDoc", 0, 0, 10);
    ASSERT_TRUE(fetchedVectors.contains("vectors"));
    ASSERT_EQ(fetchedVectors["vectors"].size(), 2);

    for (const auto& vectorJson : fetchedVectors["vectors"]) {
        int unique_id = vectorJson["id"];
        if (unique_id == 1) {
            EXPECT_EQ(vectorJson["doc"], "This is a test document for vector 1");
        } else if (unique_id == 2) {
            EXPECT_EQ(vectorJson["doc"], "Another test document for vector 2");
        }
    }
}