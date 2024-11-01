#include "atinyvectors_c_api.h"
#include "service/SnapshotService.hpp"
#include <cstring>
#include <iostream>
#include "nlohmann/json.hpp"

// C API for SnapshotServiceManager
SnapshotServiceManager* atv_snapshot_service_manager_new() {
    return reinterpret_cast<SnapshotServiceManager*>(new atinyvectors::service::SnapshotServiceManager());
}

void atv_snapshot_service_manager_free(SnapshotServiceManager* manager) {
    delete reinterpret_cast<atinyvectors::service::SnapshotServiceManager*>(manager);
}

void atv_snapshot_service_create_snapshot(SnapshotServiceManager* manager, const char* jsonStr) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SnapshotServiceManager*>(manager);
        cppManager->createSnapshot(jsonStr);
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

void atv_snapshot_service_restore_snapshot(SnapshotServiceManager* manager, const char* fileName) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SnapshotServiceManager*>(manager);
        cppManager->restoreSnapshot(fileName);
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

void atv_snapshot_service_delete_snapshot(SnapshotServiceManager* manager, const char* fileName) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SnapshotServiceManager*>(manager);
        cppManager->deleteSnapshot(fileName);
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}

char* atv_snapshot_service_list_snapshots(SnapshotServiceManager* manager) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SnapshotServiceManager*>(manager);
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

void atv_snapshot_service_delete_snapshots(SnapshotServiceManager* manager) {
    try {
        auto* cppManager = reinterpret_cast<atinyvectors::service::SnapshotServiceManager*>(manager);
        cppManager->deleteSnapshots();
    } catch (const std::exception& e) {
        atv_create_error_json(ATVErrorCode::UNKNOWN_ERROR, e.what());
    }
}
