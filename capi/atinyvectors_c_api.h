#ifndef ATINYVECTORS_C_API_H
#define ATINYVECTORS_C_API_H

#include <stddef.h>  // For size_t

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of the DTO Managers in the C API
typedef struct SearchDTOManager SearchDTOManager;
typedef struct SpaceDTOManager SpaceDTOManager;
typedef struct VectorDTOManager VectorDTOManager;
typedef struct VersionDTOManager VersionDTOManager;  // Added for VersionDTO

// C API for SearchDTOManager
SearchDTOManager* search_dto_manager_new();
void search_dto_manager_free(SearchDTOManager* manager);
char* search_dto_search(SearchDTOManager* manager, const char* spaceName, const char* queryJsonStr, size_t k);

// C API for SpaceDTOManager
SpaceDTOManager* space_dto_manager_new();
void space_dto_manager_free(SpaceDTOManager* manager);
void space_dto_create_space(SpaceDTOManager* manager, const char* jsonStr);
char* space_dto_get_by_space_id(SpaceDTOManager* manager, int spaceId);
char* space_dto_get_by_space_name(SpaceDTOManager* manager, const char* spaceName);
char* space_dto_get_lists(SpaceDTOManager* manager);

// C API for VersionDTOManager
VersionDTOManager* version_dto_manager_new();
void version_dto_manager_free(VersionDTOManager* manager);
void version_dto_create_version(VersionDTOManager* manager, const char* spaceName, const char* jsonStr);
char* version_dto_get_by_version_id(VersionDTOManager* manager, const char* spaceName, int versionId);
char* version_dto_get_by_version_name(VersionDTOManager* manager, const char* spaceName, const char* versionName);
char* version_dto_get_default_version(VersionDTOManager* manager, const char* spaceName);
char* version_dto_get_lists(VersionDTOManager* manager, const char* spaceName);

// C API for VectorDTOManager
VectorDTOManager* vector_dto_manager_new();
void vector_dto_manager_free(VectorDTOManager* manager);
void vector_dto_upsert(VectorDTOManager* manager, const char* spaceName, int versionId, const char* jsonStr);
char* vector_dto_get_vectors_by_version_id(VectorDTOManager* manager, int versionId);

// Function to free JSON string memory returned by the C API
void free_json_string(char* jsonStr);

#ifdef __cplusplus
}
#endif

#endif // ATINYVECTORS_C_API_H
