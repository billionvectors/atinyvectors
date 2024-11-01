#ifndef ATINYVECTORS_C_API_H
#define ATINYVECTORS_C_API_H

#include <stddef.h>
#include "ErrorCode.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of the Service Managers in the C API
typedef struct SearchServiceManager SearchServiceManager;
typedef struct SpaceServiceManager SpaceServiceManager;
typedef struct VectorServiceManager VectorServiceManager;
typedef struct VersionServiceManager VersionServiceManager;
typedef struct SnapshotServiceManager SnapshotServiceManager;
typedef struct RbacTokenServiceManager RbacTokenServiceManager;

// C API for SpaceServiceManager
SpaceServiceManager* atv_space_service_manager_new();
void atv_space_service_manager_free(SpaceServiceManager* manager);
void atv_space_service_create_space(SpaceServiceManager* manager, const char* jsonStr);
void atv_space_service_update_space(SpaceServiceManager* manager, const char* spaceName, const char* jsonStr);
char* atv_space_service_get_by_space_id(SpaceServiceManager* manager, int spaceId);
char* atv_space_service_get_by_space_name(SpaceServiceManager* manager, const char* spaceName);
char* atv_space_service_get_lists(SpaceServiceManager* manager);
void atv_space_service_delete_space(SpaceServiceManager* manager, const char* spaceName, const char* jsonStr);

// C API for VersionServiceManager
VersionServiceManager* atv_version_service_manager_new();
void atv_version_service_manager_free(VersionServiceManager* manager);
void atv_version_service_create_version(VersionServiceManager* manager, const char* spaceName, const char* jsonStr);
char* atv_version_service_get_by_version_id(VersionServiceManager* manager, const char* spaceName, int versionId);
char* atv_version_service_get_by_version_name(VersionServiceManager* manager, const char* spaceName, const char* versionName);
char* atv_version_service_get_default_version(VersionServiceManager* manager, const char* spaceName);
char* atv_version_service_get_lists(VersionServiceManager* manager, const char* spaceName);

// C API for VectorServiceManager
VectorServiceManager* atv_vector_service_manager_new();
void atv_vector_service_manager_free(VectorServiceManager* manager);
void atv_vector_service_upsert(VectorServiceManager* manager, const char* spaceName, int versionId, const char* jsonStr);
char* atv_vector_service_get_vectors_by_version_id(VectorServiceManager* manager, const char* spaceName, int versionId, int start, int limit);

// C API for SearchServiceManager
SearchServiceManager* atv_search_service_manager_new();
void atv_search_service_manager_free(SearchServiceManager* manager);
char* atv_search_service_search(SearchServiceManager* manager, const char* spaceName, const int versionUniqueId, const char* queryJsonStr, size_t k);

// C API for SnapshotServiceManager
SnapshotServiceManager* atv_snapshot_service_manager_new();
void atv_snapshot_service_manager_free(SnapshotServiceManager* manager);
void atv_snapshot_service_create_snapshot(SnapshotServiceManager* manager, const char* jsonStr);
void atv_snapshot_service_restore_snapshot(SnapshotServiceManager* manager, const char* fileName);
void atv_snapshot_service_delete_snapshot(SnapshotServiceManager* manager, const char* fileName);
char* atv_snapshot_service_list_snapshots(SnapshotServiceManager* manager);
void atv_snapshot_service_delete_snapshots(SnapshotServiceManager* manager);

// C API for RbacTokenServiceManager
RbacTokenServiceManager* atv_rbac_token_service_manager_new();
void atv_rbac_token_service_manager_free(RbacTokenServiceManager* manager);
int atv_rbac_token_get_system_permission(RbacTokenServiceManager* manager, const char* token);
int atv_rbac_token_get_space_permission(RbacTokenServiceManager* manager, const char* token);
int atv_rbac_token_get_version_permission(RbacTokenServiceManager* manager, const char* token);
int atv_rbac_token_get_vector_permission(RbacTokenServiceManager* manager, const char* token);
int atv_rbac_token_get_snapshot_permission(RbacTokenServiceManager* manager, const char* token);
int atv_rbac_token_get_search_permission(RbacTokenServiceManager* manager, const char* token);
int atv_rbac_token_get_security_permission(RbacTokenServiceManager* manager, const char* token);
int atv_rbac_token_get_keyvalue_permission(RbacTokenServiceManager* manager, const char* token);
char* atv_rbac_token_new_token(RbacTokenServiceManager* manager, const char* jsonStr, const char* token);
char* atv_rbac_token_list_tokens(RbacTokenServiceManager* manager);
void atv_rbac_token_delete_token(RbacTokenServiceManager* manager, const char* token);
void atv_rbac_token_update_token(RbacTokenServiceManager* manager, const char* token, const char* jsonStr);
char* atv_rbac_token_generate_jwt_token(RbacTokenServiceManager* manager, int expireDays);

// Function to free JSON string memory returned by the C API
void atv_init();
char* atv_create_error_json(ATVErrorCode code, const char* message);
void atv_free_json_string(char* jsonStr);

#ifdef __cplusplus
}
#endif

#endif // ATINYVECTORS_C_API_H
