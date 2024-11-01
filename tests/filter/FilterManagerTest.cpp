#include <gtest/gtest.h>
#include "filter/FilterManager.hpp"

using namespace atinyvectors;
using namespace atinyvectors::filter;

class FilterManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST(FilterManagerTest, ConvertFilterToSQL) {
    FilterManager& manager = FilterManager::getInstance();
    
    std::string filter = "age > 30 AND is_active == true";
    std::string expectedSQL = "(EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'age' AND vm0.value > 30) "
                              "AND EXISTS (SELECT 1 FROM VectorMetadata vm1 WHERE vm1.vectorId = VectorMetadata.vectorId AND vm1.key = 'is_active' AND vm1.value = 1))";

    std::string actualSQL = manager.toSQL(filter);

    EXPECT_EQ(actualSQL, expectedSQL);
}

TEST(FilterManagerTest, ConvertFilterToSQLWithLike) {
    FilterManager& manager = FilterManager::getInstance();

    std::string filter = "name LIKE 'John%'";
    std::string expectedSQL = "EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'name' AND vm0.value LIKE 'John%')";

    std::string actualSQL = manager.toSQL(filter);

    EXPECT_EQ(actualSQL, expectedSQL);
}

TEST(FilterManagerTest, ConvertFilterToSQLWithIn) {
    FilterManager& manager = FilterManager::getInstance();

    std::string filter = "status IN ('active', 'inactive')";
    std::string expectedSQL = "EXISTS (SELECT 1 FROM VectorMetadata vm0 WHERE vm0.vectorId = VectorMetadata.vectorId AND vm0.key = 'status' AND vm0.value IN ('active', 'inactive'))";

    std::string actualSQL = manager.toSQL(filter);

    EXPECT_EQ(actualSQL, expectedSQL);
}
