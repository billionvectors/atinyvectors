#include "atinyvectors_c_api.h"
#include "gtest/gtest.h"
#include <cstring>

class AtinyVectorsApiTest : public ::testing::Test {
protected:
    SearchDTOManager* searchManager;
    SpaceDTOManager* spaceManager;

    void SetUp() override {
        // Initialize the managers
        searchManager = search_dto_manager_new();
        spaceManager = space_dto_manager_new();
    }

    void TearDown() override {
        // Clean up the managers
        search_dto_manager_free(searchManager);
        space_dto_manager_free(spaceManager);
    }
};

TEST_F(AtinyVectorsApiTest, TestCreateSpaceAndGetBySpaceId) {
    const char* spaceJson = R"({
        "name": "test_space",
        "description": "A test space for unit tests",
        "dense": {
            "dimension": 128,
            "metric": "cosine",
            "hnsw_config": {
                "m": 16,
                "ef_construct": 200
            },
            "quantization_config": {
                "scalar": {
                    "type": "int8",
                    "quantile": 0.99,
                    "always_ram": true
                }
            }
        },
        "sparse": {
            "metric": "l2"
        }
    })";

    // Create a space
    space_dto_create_space(spaceManager, spaceJson);

    // Extract the space to JSON
    char* resultJson = space_dto_get_by_space_id(spaceManager, 1); // Assuming space ID 1 for this test

    // Print the result for debugging
    std::cout << "Extracted Space JSON: " << resultJson << std::endl;

    // Verify that the result contains the expected space name
    ASSERT_NE(resultJson, nullptr);
    EXPECT_TRUE(strstr(resultJson, "\"name\":\"test_space\"") != nullptr);

    // Free the result JSON memory
    free_json_string(resultJson);
}

TEST_F(AtinyVectorsApiTest, TestGetBySpaceName) {
    const char* spaceJson = R"({
        "name": "TestSpaceByName",
        "dimension": 128,
        "metric": "cosine",
        "dense": {
            "dimension": 1536,
            "metric": "Cosine"
        }
    })";

    space_dto_create_space(spaceManager, spaceJson);

    // Query the space by name
    char* resultJson = space_dto_get_by_space_name(spaceManager, "TestSpaceByName");

    ASSERT_NE(resultJson, nullptr);
    ASSERT_TRUE(strstr(resultJson, "\"name\":\"TestSpaceByName\"") != nullptr);

    free_json_string(resultJson);
}

TEST_F(AtinyVectorsApiTest, TestGetLists) {
    // Create multiple spaces for testing
    const char* spaceJson1 = R"({"name": "TestSpace1"})";
    const char* spaceJson2 = R"({"name": "TestSpace2"})";

    space_dto_create_space(spaceManager, spaceJson1);
    space_dto_create_space(spaceManager, spaceJson2);

    // Get the list of all spaces
    char* resultJson = space_dto_get_lists(spaceManager);

    // Print the result for debugging
    std::cout << "List of Spaces JSON: " << resultJson << std::endl;

    // Verify that the result contains the expected space names
    ASSERT_NE(resultJson, nullptr);
    EXPECT_TRUE(strstr(resultJson, "\"TestSpace1\"") != nullptr);
    EXPECT_TRUE(strstr(resultJson, "\"TestSpace2\"") != nullptr);

    // Free the result JSON memory
    free_json_string(resultJson);
}
