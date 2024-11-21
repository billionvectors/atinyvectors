#include <gtest/gtest.h>
#include "service/SpaceService.hpp"
#include "service/VectorService.hpp"
#include "algo/FaissIndexLRUCache.hpp"
#include "utils/Utils.hpp"
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
class SpaceServiceManagerTest : public ::testing::Test {
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
        FaissIndexLRUCache::getInstance().clean();
    }

    void TearDown() override {
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Snapshot;");
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }
};

TEST_F(SpaceServiceManagerTest, CreateSpaceWithNormalizedJson) {
    std::string inputJson = R"({
        "name": "spacename",
        "dimension": 128,
        "metric": "cosine",
        "hnsw_config": {
            "ef_construct": 123
        },
        "quantization_config": {
            "scalar": {
                "type": "int8"
            }
        },
        "dense": {
            "dimension": 1536,
            "metric": "Cosine",
            "hnsw_config": {
                "m": 32,
                "ef_construct": 123
            },
            "quantization_config": {
                "scalar": {
                    "type": "int8"
                }
            }
        },
        "sparse": {
            "metric": "Cosine"
        },
        "indexes": {
            "index1": {
                "dimension": 4,
                "metric": "Cosine",
                "hnsw_config": {
                    "m": 20
                },
                "quantization_config": {
                    "scalar": {
                        "type": "int8"
                    }
                }
            },
            "index2": {
                "dimension": 4,
                "metric": "Cosine",
                "hnsw_config": {
                    "m": 20
                },
                "quantization_config": {
                    "scalar": {
                        "type": "int8"
                    }
                }
            }
        }
    })";

    SpaceServiceManager manager;
    manager.createSpace(inputJson);

    auto spaces = SpaceManager::getInstance().getAllSpaces();
    ASSERT_EQ(spaces.size(), 1);

    auto versions = VersionManager::getInstance().getVersionsBySpaceId(spaces[0].id);
    ASSERT_EQ(versions.size(), 1);

    auto vectorIndexes = VectorIndexManager::getInstance().getVectorIndicesByVersionId(versions[0].id);
    ASSERT_EQ(vectorIndexes.size(), 4);

    for (const auto& vectorIndex : vectorIndexes) {
        if (vectorIndex.name == Config::getInstance().getDefaultDenseIndexName()) {

            auto hnswConfig = vectorIndex.getHnswConfig();
            auto quantizationConfig = vectorIndex.getQuantizationConfig();
            EXPECT_EQ(hnswConfig.M, 32);
            EXPECT_EQ(hnswConfig.EfConstruct, 123);

            spdlog::info("quantizationConfig: QuantizationType {}", (int)quantizationConfig.QuantizationType);
            EXPECT_EQ(quantizationConfig.Scalar.Type, "int8");
        } else if (vectorIndex.name == Config::getInstance().getDefaultSparseIndexName()) {
            EXPECT_EQ(vectorIndex.metricType, MetricType::Cosine);
        } else if (vectorIndex.name == "index1") {
            auto hnswConfig = vectorIndex.getHnswConfig();
            auto quantizationConfig = vectorIndex.getQuantizationConfig();
            EXPECT_EQ(hnswConfig.M, 20);
            spdlog::info("quantizationConfig: QuantizationType {}", (int)quantizationConfig.QuantizationType);
            EXPECT_EQ(quantizationConfig.Scalar.Type, "int8");
        } else if (vectorIndex.name == "index2") {
            auto hnswConfig = vectorIndex.getHnswConfig();
            auto quantizationConfig = vectorIndex.getQuantizationConfig();
            EXPECT_EQ(hnswConfig.M, 20);
            spdlog::info("quantizationConfig: QuantizationType {}", (int)quantizationConfig.QuantizationType);
            EXPECT_EQ(quantizationConfig.Scalar.Type, "int8");
        }
    }
}

TEST_F(SpaceServiceManagerTest, GetBySpaceIdTest) {
    std::string inputJson = R"({
        "name": "GetBySpaceIdTest",
        "dimension": 128,
        "metric": "cosine",
        "dense": {
            "dimension": 1536,
            "metric": "Cosine",
            "hnsw_config": {
                "m": 32,
                "ef_construct": 123
            },
            "quantization_config": {
                "scalar": {
                    "type": "int8",
                    "quantile": 0.8
                }
            }
        },
        "sparse": {
            "metric": "Cosine"
        }
    })";

    SpaceServiceManager manager;
    manager.createSpace(inputJson);

    // Get all spaces and search for the one with the name "GetBySpaceIdTest"
    auto spaces = SpaceManager::getInstance().getAllSpaces();
    ASSERT_FALSE(spaces.empty());  // Ensure spaces are returned

    // Find the space with the name "GetBySpaceIdTest"
    int spaceId = -1;
    for (const auto& space : spaces) {
        if (space.name == "GetBySpaceIdTest") {
            spaceId = space.id;
            break;
        }
    }

    ASSERT_NE(spaceId, -1);  // Ensure we found the space by name

    // Extract JSON data for the space by its ID
    json extractedJson = manager.getBySpaceId(spaceId);
    ASSERT_EQ(extractedJson["spaceId"], spaceId);
    ASSERT_TRUE(extractedJson.contains("version"));
    ASSERT_TRUE(extractedJson["version"].contains("vectorIndices"));

    // Get vector indices and validate their properties
    const auto& vectorIndices = extractedJson["version"]["vectorIndices"];
    ASSERT_EQ(vectorIndices.size(), 2);  // We expect 2 vector indices (dense, sparse)

    for (const auto& vectorIndex : vectorIndices) {
        ASSERT_TRUE(vectorIndex.contains("vectorIndexId"));
        ASSERT_TRUE(vectorIndex.contains("vectorValueType"));
        ASSERT_TRUE(vectorIndex.contains("name"));
        ASSERT_TRUE(vectorIndex.contains("created_time_utc"));
        ASSERT_TRUE(vectorIndex.contains("updated_time_utc"));
        ASSERT_TRUE(vectorIndex.contains("is_default"));

        std::string indexName = vectorIndex["name"];

        if (indexName == Config::getInstance().getDefaultDenseIndexName()) {
            EXPECT_EQ(vectorIndex["vectorValueType"], static_cast<int>(VectorValueType::Dense));
            EXPECT_EQ(vectorIndex["metricType"], static_cast<int>(MetricType::Cosine));
            EXPECT_EQ(vectorIndex["dimension"], 1536);
            EXPECT_EQ(vectorIndex["hnswConfig"]["M"], 32);
            EXPECT_EQ(vectorIndex["hnswConfig"]["EfConstruct"], 123);
        } else if (indexName == Config::getInstance().getDefaultSparseIndexName()) {
            EXPECT_EQ(vectorIndex["vectorValueType"], static_cast<int>(VectorValueType::Sparse));
            EXPECT_EQ(vectorIndex["metricType"], static_cast<int>(MetricType::Cosine));
        } else {
            FAIL() << "Unexpected vector index name: " << indexName;
        }
    }
}

TEST_F(SpaceServiceManagerTest, GetListsTest) {
    // Create multiple spaces
    std::string inputJson1 = R"({
        "name": "space1",
        "dimension": 128,
        "metric": "cosine"
    })";

    std::string inputJson2 = R"({
        "name": "space2",
        "dimension": 128,
        "metric": "cosine"
    })";

    SpaceServiceManager manager;
    manager.createSpace(inputJson1);
    manager.createSpace(inputJson2);

    // Get the list of all spaces as JSON
    json spaceListJson = manager.getLists();

    // Check if the JSON contains the values key
    ASSERT_TRUE(spaceListJson.contains("values"));
    
    // Check if two spaces were created
    auto valuesArray = spaceListJson["values"];
    ASSERT_EQ(valuesArray.size(), 2);

    // Check if the correct space names and IDs are returned
    bool space1Found = false;
    bool space2Found = false;

    for (const auto& spaceEntry : valuesArray) {
        if (spaceEntry["name"] == "space1") {
            space1Found = true;
        }
        if (spaceEntry["name"] == "space2") {
            space2Found = true;
        }
    }

    EXPECT_TRUE(space1Found);
    EXPECT_TRUE(space2Found);
}

TEST_F(SpaceServiceManagerTest, GetBySpaceNameTest) {
    // Create a space
    std::string inputJson = R"({
        "name": "GetBySpaceNameTest",
        "dimension": 128,
        "metric": "cosine"
    })";

    SpaceServiceManager manager;
    manager.createSpace(inputJson);

    // Retrieve the space by name
    json spaceJson = manager.getBySpaceName("GetBySpaceNameTest");

    // Validate the JSON output
    ASSERT_TRUE(spaceJson.contains("spaceId"));
    ASSERT_EQ(spaceJson["name"], "GetBySpaceNameTest");
    ASSERT_TRUE(spaceJson.contains("created_time_utc"));
    ASSERT_TRUE(spaceJson.contains("updated_time_utc"));
}

TEST_F(SpaceServiceManagerTest, DeleteTest) {
    // Set up Space, Version, and VectorIndex for search
    Space defaultSpace(0, "DeleteTest", "Default Space Description", 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(defaultSpace);

    // Use versionUniqueId as 1 for the test case
    Version defaultVersion(0, spaceId, 1, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    // Cache the versionId using IdCache
    IdCache::getInstance().getVersionId("DeleteTest", 1); // Assuming this populates the cache

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

    VectorServiceManager VectorServiceManager;
    VectorServiceManager.upsert("DeleteTest", 1, vectorDataJson);

    SpaceServiceManager SpaceServiceManager;
    SpaceServiceManager.deleteSpace("DeleteTest", "{}");
    
    EXPECT_THROW(SpaceManager::getInstance().getSpaceById(spaceId), std::runtime_error);
    EXPECT_THROW(VersionManager::getInstance().getVersionById(versionId), std::runtime_error);
}

TEST_F(SpaceServiceManagerTest, UpdateSpaceTest) {
    std::string initialJson = R"({
        "name": "UpdateSpaceTest",
        "dimension": 128,
        "metric": "cosine",
        "dense": {
            "dimension": 1536,
            "metric": "Cosine",
            "hnsw_config": {
                "m": 32,
                "ef_construct": 123
            },
            "quantization_config": {
                "scalar": {
                    "type": "int8"
                }
            }
        },
        "sparse": {
            "metric": "Cosine"
        }
    })";

    SpaceServiceManager manager;
    manager.createSpace(initialJson);
    int spaceId = IdCache::getInstance().getSpaceId("UpdateSpaceTest");

    std::string updateJsonStr = R"({
        "dense": {
            "dimension": 1234,
            "metric": "l2",
            "hnsw_config": {
                "m": 64,
                "ef_construct": 55
            }
        }
    })";

    manager.updateSpace("UpdateSpaceTest", updateJsonStr);

    json updatedJson = manager.getBySpaceId(spaceId);
    
    auto versions = VersionManager::getInstance().getVersionsBySpaceId(spaceId);
    ASSERT_EQ(versions.size(), 1);
    int versionId = versions[0].id;

    auto vectorIndexes = VectorIndexManager::getInstance().getVectorIndicesByVersionId(versionId);
    ASSERT_EQ(vectorIndexes.size(), 2);

    for (const auto& vectorIndex : vectorIndexes) {
        if (vectorIndex.name == Config::getInstance().getDefaultDenseIndexName()) {
            auto hnswConfig = vectorIndex.getHnswConfig();
            auto quantizationConfig = vectorIndex.getQuantizationConfig();
            EXPECT_EQ(vectorIndex.dimension, 1234);
            EXPECT_EQ(vectorIndex.metricType, MetricType::L2);
            EXPECT_EQ(hnswConfig.M, 64);
            EXPECT_EQ(hnswConfig.EfConstruct, 55);
            EXPECT_EQ(quantizationConfig.Scalar.Type, "int8");
        } else if (vectorIndex.name == Config::getInstance().getDefaultSparseIndexName()) {
            EXPECT_EQ(vectorIndex.metricType, MetricType::Cosine);
            EXPECT_EQ(vectorIndex.dimension, 0); // Sparse index can be no dimension
        } else {
            FAIL() << "Unexpected vector index name: " << vectorIndex.name;
        }
    }
}

TEST_F(SpaceServiceManagerTest, UpdateSpaceWithAssignedVectorsShouldThrow) {
    std::string initialJson = R"({
        "name": "UpdateSpaceTest",
        "dimension": 4,
        "metric": "cosine"
    })";

    SpaceServiceManager manager;
    manager.createSpace(initialJson);
    int spaceId = IdCache::getInstance().getSpaceId("UpdateSpaceTest");
    ASSERT_GT(spaceId, 0) << "Failed to create space.";

    VectorServiceManager VectorServiceManager;
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

    VectorServiceManager.upsert("UpdateSpaceTest", 1, vectorDataJson);

    std::string updateJsonStr = R"({
        "dense": {
            "dimension": 1234,
            "metric": "l2",
            "hnsw_config": {
                "m": 64,
                "ef_construct": 55
            }
        }
    })";

    EXPECT_THROW({
        manager.updateSpace("UpdateSpaceTest", updateJsonStr);
    }, std::runtime_error) << "updateSpace did not throw an exception when vectors are assigned.";
}
