// VectorDTOTest.cpp

#include <gtest/gtest.h>
#include "dto/VectorDTO.hpp"
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorIndexOptimizer.hpp"
#include "VectorMetadata.hpp"
#include "Version.hpp"
#include "Space.hpp"
#include "DatabaseManager.hpp"
#include "nlohmann/json.hpp"

using namespace atinyvectors;
using namespace atinyvectors::dto;
using namespace nlohmann;

// Test Fixture class
class VectorDTOManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        SpaceManager::getInstance();
        VectorIndexOptimizerManager::getInstance();
        VectorIndexManager::getInstance();
        VersionManager::getInstance();
        VectorMetadataManager::getInstance();
        VectorManager::getInstance();

        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndexOptimizer;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }

    void TearDown() override {
        auto& db = DatabaseManager::getInstance().getDatabase();
        db.exec("DELETE FROM Space;");
        db.exec("DELETE FROM VectorIndexOptimizer;");
        db.exec("DELETE FROM VectorIndex;");
        db.exec("DELETE FROM Version;");
        db.exec("DELETE FROM VectorMetadata;");
        db.exec("DELETE FROM Vector;");
        db.exec("DELETE FROM VectorValue;");
    }
};

TEST_F(VectorDTOManagerTest, UpsertVectorsWithJson) {
    // Set up necessary default Space and Version
    Space defaultSpace(0, "default_space", "Default Space Description", 0, 0);
    int spaceId = SpaceManager::getInstance().addSpace(defaultSpace);

    Version defaultVersion(0, spaceId, 0, "Default Version", "Automatically created default version", "v1", 0, 0, true);
    int versionId = VersionManager::getInstance().addVersion(defaultVersion);

    // Create a default VectorIndex if none exists
    auto vectorIndices = VectorIndexManager::getInstance().getVectorIndicesByVersionId(versionId);
    int defaultIndexId = 0;
    if (vectorIndices.empty()) {
        VectorIndex defaultIndex(0, versionId, VectorValueType::Dense, "Default Index", 0, 0, true);
        defaultIndexId = VectorIndexManager::getInstance().addVectorIndex(defaultIndex);
    } else {
        defaultIndexId = vectorIndices[0].id;
    }

    // Add a VectorIndexOptimizer entry for the defaultIndexId
    SQLite::Statement optimizerQuery(DatabaseManager::getInstance().getDatabase(),
                                     "INSERT INTO VectorIndexOptimizer (vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson) "
                                     "VALUES (?, ?, ?, ?, ?)");
    optimizerQuery.bind(1, defaultIndexId);
    optimizerQuery.bind(2, static_cast<int>(MetricType::L2));  // Use L2 metric type
    optimizerQuery.bind(3, 128);  // Dimension size of 128
    optimizerQuery.bind(4, R"({"M": 16, "EfConstruct": 200})");  // Example HnswConfig
    optimizerQuery.bind(5, "{}");  // Empty quantization config
    optimizerQuery.exec();

    // Multiple operations including vector insertion and upsert
    std::string updateJson = R"({
        "vectors": [
            {
                "id": 2,
                "data": [0.20, 0.62, 0.77, 0.75],
                "metadata": {"city": "New York"}
            }
        ]
    })";

    VectorDTOManager manager;
    manager.upsert("default_space", 0, updateJson);

    // Fetch using getVectorByUniqueId
    auto vector1 = VectorManager::getInstance().getVectorByUniqueId(versionId, 2);
    ASSERT_EQ(vector1.unique_id, 2);  // Check unique_id
    EXPECT_EQ(vector1.values[0].denseData, std::vector<float>({0.20f, 0.62f, 0.77f, 0.75f}));

    // Check updated metadata for vector with unique_id 2
    auto metadataList1 = VectorMetadataManager::getInstance().getVectorMetadataByVectorId(vector1.id);
    ASSERT_EQ(metadataList1.size(), 1); // Only one metadata entry should exist
    EXPECT_EQ(metadataList1[0].key, "city");
    EXPECT_EQ(metadataList1[0].value, "New York");
}
