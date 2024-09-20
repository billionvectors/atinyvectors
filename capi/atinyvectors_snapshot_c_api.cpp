#include "atinyvectors_c_api.h"
#include "dto/SnapshotDTO.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

// C API for SnapshotDTOManager
SnapshotDTOManager* atv_snapshot_dto_manager_new() {
    return reinterpret_cast<SnapshotDTOManager*>(new atinyvectors::dto::SnapshotDTOManager());
}

void atv_snapshot_dto_manager_free(SnapshotDTOManager* manager) {
    delete reinterpret_cast<atinyvectors::dto::SnapshotDTOManager*>(manager);
}

void atv_snapshot_dto_create_snapshot(SnapshotDTOManager* manager, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SnapshotDTOManager*>(manager);
        cppManager->createSnapshot(jsonStr);
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

void atv_snapshot_dto_restore_snapshot(SnapshotDTOManager* manager, const char* fileName) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SnapshotDTOManager*>(manager);
        cppManager->restoreSnapshot(fileName);
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_snapshot_dto_list_snapshots(SnapshotDTOManager* manager) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SnapshotDTOManager*>(manager);
        nlohmann::json result = cppManager->listSnapshots();
        
        // Allocate memory and return JSON string
        std::string jsonString = result.dump();
        char* resultCStr = (char*)malloc(jsonString.size() + 1);
        std::strcpy(resultCStr, jsonString.c_str());
        return resultCStr;
    } catch (const nlohmann::json::exception& e) {
        return atv_create_error_json(ATVErrorCode::JSON_PARSE_ERROR, e.what());
    } catch (const std::exception& e) {
        return atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

void atv_snapshot_dto_delete_snapshots(SnapshotDTOManager* manager) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::dto::SnapshotDTOManager*>(manager);
        cppManager->deleteSnapshots();
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}
