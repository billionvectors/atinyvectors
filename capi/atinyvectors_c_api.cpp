#include "atinyvectors_c_api.h"
#include "Config.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

void atv_init() {
    atinyvectors::Config::getInstance().reset();
    atinyvectors::Config::getInstance();

    spdlog::info("atinyvectors has been initialized");
}

char* atv_create_error_json(ATVErrorCode code, const char* message) {
    nlohmann::json errorJson;
    errorJson["error"]["code"] = static_cast<int>(code);
    errorJson["error"]["message"] = message;
    std::string jsonString = errorJson.dump();
    char* resultCStr = (char*)malloc(jsonString.size() + 1);
    std::strcpy(resultCStr, jsonString.c_str());
    return resultCStr;
}

// Function to free JSON string memory
void atv_free_json_string(char* jsonStr) {
    free(jsonStr);
}
