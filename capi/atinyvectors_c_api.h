#ifndef ATINYVECTORS_C_API_H
#define ATINYVECTORS_C_API_H

#include <stddef.h>
#include "ErrorCode.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of the DTO Managers in the C API
typedef struct SearchDTOManager SearchDTOManager;
typedef struct SpaceDTOManager SpaceDTOManager;
typedef struct VectorDTOManager VectorDTOManager;
typedef struct VersionDTOManager VersionDTOManager;
typedef struct SnapshotDTOManager SnapshotDTOManager;
typedef struct RbacTokenDTOManager RbacTokenDTOManager;

// C API for SpaceDTOManager
SpaceDTOManager* atv_space_dto_manager_new();
void atv_space_dto_manager_free(SpaceDTOManager* manager);
void atv_space_dto_create_space(SpaceDTOManager* manager, const char* jsonStr);
void atv_space_dto_update_space(SpaceDTOManager* manager, const char* spaceName, const char* jsonStr);
char* atv_space_dto_get_by_space_id(SpaceDTOManager* manager, int spaceId);
char* atv_space_dto_get_by_space_name(SpaceDTOManager* manager, const char* spaceName);
char* atv_space_dto_get_lists(SpaceDTOManager* manager);
void atv_space_dto_delete_space(SpaceDTOManager* manager, const char* spaceName, const char* jsonStr);

// C API for VersionDTOManager
VersionDTOManager* atv_version_dto_manager_new();
void atv_version_dto_manager_free(VersionDTOManager* manager);
void atv_version_dto_create_version(VersionDTOManager* manager, const char* spaceName, const char* jsonStr);
char* atv_version_dto_get_by_version_id(VersionDTOManager* manager, const char* spaceName, int versionId);
char* atv_version_dto_get_by_version_name(VersionDTOManager* manager, const char* spaceName, const char* versionName);
char* atv_version_dto_get_default_version(VersionDTOManager* manager, const char* spaceName);
char* atv_version_dto_get_lists(VersionDTOManager* manager, const char* spaceName);

// C API for VectorDTOManager
VectorDTOManager* atv_vector_dto_manager_new();
void atv_vector_dto_manager_free(VectorDTOManager* manager);
void atv_vector_dto_upsert(VectorDTOManager* manager, const char* spaceName, int versionId, const char* jsonStr);
char* atv_vector_dto_get_vectors_by_version_id(VectorDTOManager* manager, int versionId);

// C API for SearchDTOManager
SearchDTOManager* atv_search_dto_manager_new();
void atv_search_dto_manager_free(SearchDTOManager* manager);
char* atv_search_dto_search(SearchDTOManager* manager, const char* spaceName, const int versionUniqueId, const char* queryJsonStr, size_t k);

// C API for SnapshotDTOManager
SnapshotDTOManager* atv_snapshot_dto_manager_new();
void atv_snapshot_dto_manager_free(SnapshotDTOManager* manager);
void atv_snapshot_dto_create_snapshot(SnapshotDTOManager* manager, const char* jsonStr);
void atv_snapshot_dto_restore_snapshot(SnapshotDTOManager* manager, const char* fileName);
char* atv_snapshot_dto_list_snapshots(SnapshotDTOManager* manager);
void atv_snapshot_dto_delete_snapshots(SnapshotDTOManager* manager);

// C API for RbacTokenDTOManager
RbacTokenDTOManager* atv_rbac_token_dto_manager_new();
void atv_rbac_token_dto_manager_free(RbacTokenDTOManager* manager);
int atv_rbac_token_get_system_permission(RbacTokenDTOManager* manager, const char* token);
int atv_rbac_token_get_space_permission(RbacTokenDTOManager* manager, const char* token);
int atv_rbac_token_get_version_permission(RbacTokenDTOManager* manager, const char* token);
int atv_rbac_token_get_vector_permission(RbacTokenDTOManager* manager, const char* token);
int atv_rbac_token_get_snapshot_permission(RbacTokenDTOManager* manager, const char* token);
int atv_rbac_token_get_search_permission(RbacTokenDTOManager* manager, const char* token);
int atv_rbac_token_get_security_permission(RbacTokenDTOManager* manager, const char* token);
int atv_rbac_token_get_keyvalue_permission(RbacTokenDTOManager* manager, const char* token);
char* atv_rbac_token_new_token(RbacTokenDTOManager* manager, const char* jsonStr, const char* token);
char* atv_rbac_token_list_tokens(RbacTokenDTOManager* manager);
void atv_rbac_token_delete_token(RbacTokenDTOManager* manager, const char* token);
void atv_rbac_token_update_token(RbacTokenDTOManager* manager, const char* token, const char* jsonStr);
char* atv_rbac_token_generate_jwt_token(RbacTokenDTOManager* manager, int expireDays);

// Function to free JSON string memory returned by the C API
void atv_init();
char* atv_create_error_json(ATVErrorCode code, const char* message);
void atv_free_json_string(char* jsonStr);

#ifdef __cplusplus
}
#endif

#endif // ATINYVECTORS_C_API_H
