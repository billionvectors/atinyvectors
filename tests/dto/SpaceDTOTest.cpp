#include <gtest/gtest.h>
#include "dto/SpaceDTO.hpp"
#include "algo/HnswIndexLRUCache.hpp"
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
using namespace atinyvectors::dto;
using namespace atinyvectors::algo;
using namespace atinyvectors::utils;
using namespace nlohmann;

// Test Fixture class
class SpaceDTOManagerTest : public ::testing::Test {
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

TEST_F(SpaceDTOManagerTest, CreateSpaceWithNormalizedJson) {
    std::string inputJson = R"({
        "name": "spacename",
        "dimension": 128,
        "metric": "cosine",
        "hnsw_config": {
            "ef_construct": 123
        },
        "quantization_config": {
            "scalar": {
                "type": "int8",
                "quantile": 0.99,
                "always_ram": true
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
                    "type": "int8",
                    "quantile": 0.8
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
                        "type": "int8",
                        "quantile": 0.6
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
                        "type": "int8",
                        "quantile": 0.6
                    }
                }
            }
        }
    })";

    SpaceDTOManager manager;
    manager.createSpace(inputJson);

    // Space 확인
    auto spaces = SpaceManager::getInstance().getAllSpaces();
    ASSERT_EQ(spaces.size(), 1);

    // Version 확인
    auto versions = VersionManager::getInstance().getVersionsBySpaceId(spaces[0].id);
    ASSERT_EQ(versions.size(), 1);

    // VectorIndex 가져오기
    auto vectorIndexes = VectorIndexManager::getInstance().getVectorIndicesByVersionId(versions[0].id);
    ASSERT_EQ(vectorIndexes.size(), 4);  // Dense, Sparse, index1, index2 인덱스 4개

    // 각 index의 이름을 확인하고, 일치하는 경우에만 검증 수행
    for (const auto& vectorIndex : vectorIndexes) {
        if (vectorIndex.name == "Default Dense Index") {
            // Dense Vector와 Optimizer 확인
            auto hnswConfig = vectorIndex.getHnswConfig();
            auto quantizationConfig = vectorIndex.getQuantizationConfig();
            EXPECT_EQ(hnswConfig.M, 32);
            EXPECT_EQ(hnswConfig.EfConstruct, 123);
            EXPECT_EQ(quantizationConfig.Scalar.Type, "int8");
            EXPECT_FLOAT_EQ(quantizationConfig.Scalar.Quantile, 0.8f);
        } else if (vectorIndex.name == "Default Sparse Index") {
            // Sparse Vector와 Optimizer 확인
            EXPECT_EQ(vectorIndex.metricType, MetricType::Cosine);
        } else if (vectorIndex.name == "index1") {
            // 추가된 Index 1 확인
            auto hnswConfig = vectorIndex.getHnswConfig();
            auto quantizationConfig = vectorIndex.getQuantizationConfig();
            EXPECT_EQ(hnswConfig.M, 20);
            EXPECT_EQ(quantizationConfig.Scalar.Type, "int8");
            EXPECT_FLOAT_EQ(quantizationConfig.Scalar.Quantile, 0.6f);
        } else if (vectorIndex.name == "index2") {
            // 추가된 Index 2 확인
            auto hnswConfig = vectorIndex.getHnswConfig();
            auto quantizationConfig = vectorIndex.getQuantizationConfig();
            EXPECT_EQ(hnswConfig.M, 20);
            EXPECT_EQ(quantizationConfig.Scalar.Type, "int8");
            EXPECT_FLOAT_EQ(quantizationConfig.Scalar.Quantile, 0.6f);
        }
    }
}

TEST_F(SpaceDTOManagerTest, GetBySpaceIdTest) {
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

    SpaceDTOManager manager;
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

        if (indexName == "Default Dense Index") {
            EXPECT_EQ(vectorIndex["vectorValueType"], static_cast<int>(VectorValueType::Dense));
            EXPECT_EQ(vectorIndex["metricType"], static_cast<int>(MetricType::Cosine));
            EXPECT_EQ(vectorIndex["dimension"], 1536);
            EXPECT_EQ(vectorIndex["hnswConfig"]["M"], 32);
            EXPECT_EQ(vectorIndex["hnswConfig"]["EfConstruct"], 123);
        } else if (indexName == "Default Sparse Index") {
            EXPECT_EQ(vectorIndex["vectorValueType"], static_cast<int>(VectorValueType::Sparse));
            EXPECT_EQ(vectorIndex["metricType"], static_cast<int>(MetricType::Cosine));
        } else {
            FAIL() << "Unexpected vector index name: " << indexName;
        }
    }
}

TEST_F(SpaceDTOManagerTest, GetListsTest) {
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

    SpaceDTOManager manager;
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
        if (spaceEntry.contains("space1")) {
            space1Found = true;
        }
        if (spaceEntry.contains("space2")) {
            space2Found = true;
        }
    }

    EXPECT_TRUE(space1Found);
    EXPECT_TRUE(space2Found);
}

TEST_F(SpaceDTOManagerTest, GetBySpaceNameTest) {
    // Create a space
    std::string inputJson = R"({
        "name": "test_space",
        "dimension": 128,
        "metric": "cosine"
    })";

    SpaceDTOManager manager;
    manager.createSpace(inputJson);

    // Retrieve the space by name
    json spaceJson = manager.getBySpaceName("test_space");

    // Validate the JSON output
    ASSERT_TRUE(spaceJson.contains("spaceId"));
    ASSERT_EQ(spaceJson["name"], "test_space");
    ASSERT_TRUE(spaceJson.contains("created_time_utc"));
    ASSERT_TRUE(spaceJson.contains("updated_time_utc"));
}
