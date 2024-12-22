#include <gtest/gtest.h>
#include "DatabaseManager.hpp"
#include "Config.hpp"
#include <filesystem>

using namespace atinyvectors;

class DatabaseManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up mock migration path
        migrationPath = "mockdb";
        DatabaseManager::getInstance(":memory:", migrationPath).reset();
    }

    std::string migrationPath;
};

TEST_F(DatabaseManagerTest, TestMigration) {
    DatabaseManager& dbManager = DatabaseManager::getInstance(":memory:", migrationPath);

    auto& db = dbManager.getDatabase();
    SQLite::Statement updateInfoQuery(db, "UPDATE info SET dbversion = 2;");
    int updatedRows = updateInfoQuery.exec();

    // Perform migration
    dbManager.migrate();

    // Verify info table has the correct version and dbversion
    SQLite::Statement query(db, "SELECT version, dbversion FROM info;");
    ASSERT_TRUE(query.executeStep());
    ASSERT_EQ(query.getColumn(0).getString(), Config::getInstance().getProjectVersion());
    ASSERT_EQ(query.getColumn(1).getInt(), 3); // Latest migration version

    SQLite::Statement checkBillionVectorsTable(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='billionvectors';");
    ASSERT_TRUE(checkBillionVectorsTable.executeStep());
}
