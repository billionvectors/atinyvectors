cmake_minimum_required(VERSION 3.14)

project(atinyvectors VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure position-independent code is enabled for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# Fetch SQLiteCpp library
FetchContent_Declare(
  SQLiteCpp
  GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
  GIT_TAG 3.3.2
)
FetchContent_MakeAvailable(SQLiteCpp)

# Fetch GoogleTest library
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Fetch spdlog library
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.11.0
)
FetchContent_MakeAvailable(spdlog)

# Fetch hnswlib library
FetchContent_Declare(
  hnswlib
  GIT_REPOSITORY https://github.com/nmslib/hnswlib.git
  GIT_TAG v0.8.0
)
FetchContent_MakeAvailable(hnswlib)

# Fetch libzip library for ZIP functionality
FetchContent_Declare(
  libzip
  GIT_REPOSITORY https://github.com/nih-at/libzip.git
  GIT_TAG v1.10.1
)
FetchContent_MakeAvailable(libzip)

# Fetch jwt-cpp library for JWT functionality
FetchContent_Declare(
  jwt-cpp
  GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
  GIT_TAG v0.7.0
)
FetchContent_MakeAvailable(jwt-cpp)

# OpenSSL
find_package(OpenSSL REQUIRED)

# Include directories for header files
include_directories(capi)
include_directories(include)
include_directories(src)

# Set compiler flags based on build type
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Build the library as a SHARED library
add_library(${PROJECT_NAME} SHARED
  capi/atinyvectors_c_api.cpp
  capi/atinyvectors_rbac_c_api.cpp
  capi/atinyvectors_search_c_api.cpp
  capi/atinyvectors_snapshot_c_api.cpp
  capi/atinyvectors_space_c_api.cpp
  capi/atinyvectors_vector_c_api.cpp
  capi/atinyvectors_version_c_api.cpp

  src/impl/algo/HnswIndexLRUCacheImpl.cpp
  src/impl/algo/HnswIndexManagerImpl.cpp
  src/impl/algo/SparseSpaceImpl.cpp

  src/impl/dto/RbacTokenDTOImpl.cpp 
  src/impl/dto/SearchDTOImpl.cpp 
  src/impl/dto/SnapshotDTOImpl.cpp 
  src/impl/dto/SpaceDTOImpl.cpp 
  src/impl/dto/VectorDTOImpl.cpp 
  src/impl/dto/VersionDTOImpl.cpp 

  src/impl/utils/UtilsImpl.cpp
  
  src/impl/ConfigImpl.cpp 
  src/impl/DatabaseManagerImpl.cpp 
  src/impl/IdCacheImpl.cpp 
  src/impl/RbacTokenImpl.cpp 
  src/impl/SnapshotImpl.cpp
  src/impl/SpaceImpl.cpp
  src/impl/SparseDataPoolImpl.cpp
  src/impl/VectorIndexImpl.cpp
  src/impl/VectorManagerImpl.cpp
  src/impl/VectorMetadataImpl.cpp
  src/impl/VectorValueImpl.cpp
  src/impl/VersionImpl.cpp
  src/impl/SnapshotImpl.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SQLiteCpp
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    hnswlib
    zip
    jwt-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Add executable for the test program
add_executable(test_${PROJECT_NAME} 
  tests/algo/HnswIndexLRUCacheTest.cpp
  tests/algo/HnswIndexManagerTest.cpp
  tests/algo/HnswLibTest.cpp
  tests/algo/SparseIpSpaceTest.cpp
  tests/algo/SparseL2SpaceTest.cpp

  tests/dto/RbacTokenDTOTest.cpp
  tests/dto/SearchDTOTest.cpp
  tests/dto/SnapshotDTOTest.cpp
  tests/dto/SpaceDTOTest.cpp
  tests/dto/VectorDTOTest.cpp
  tests/dto/VersionDTOTest.cpp
  
  tests/ConfigTest.cpp 
  tests/IdCacheTest.cpp 
  tests/RbacTokenTest.cpp 
  tests/SearchTest.cpp 
  tests/SnapshotTest.cpp 
  tests/SpaceTest.cpp 
  tests/VectorIndexTest.cpp
  tests/VectorMetadataTest.cpp
  tests/VectorTest.cpp
  tests/VersionTest.cpp

  # Source files from the library
  capi/atinyvectors_c_api.cpp
  capi/atinyvectors_rbac_c_api.cpp
  capi/atinyvectors_search_c_api.cpp
  capi/atinyvectors_snapshot_c_api.cpp
  capi/atinyvectors_space_c_api.cpp
  capi/atinyvectors_vector_c_api.cpp
  capi/atinyvectors_version_c_api.cpp

  src/impl/algo/HnswIndexLRUCacheImpl.cpp
  src/impl/algo/HnswIndexManagerImpl.cpp
  src/impl/algo/SparseSpaceImpl.cpp

  src/impl/dto/RbacTokenDTOImpl.cpp 
  src/impl/dto/SearchDTOImpl.cpp 
  src/impl/dto/SnapshotDTOImpl.cpp 
  src/impl/dto/SpaceDTOImpl.cpp 
  src/impl/dto/VectorDTOImpl.cpp 
  src/impl/dto/VersionDTOImpl.cpp 

  src/impl/utils/UtilsImpl.cpp

  src/impl/ConfigImpl.cpp 
  src/impl/DatabaseManagerImpl.cpp 
  src/impl/IdCacheImpl.cpp 
  src/impl/RbacTokenImpl.cpp 
  src/impl/SnapshotImpl.cpp
  src/impl/SpaceImpl.cpp
  src/impl/SparseDataPoolImpl.cpp
  src/impl/VectorIndexImpl.cpp
  src/impl/VectorManagerImpl.cpp
  src/impl/VectorMetadataImpl.cpp
  src/impl/VectorValueImpl.cpp
  src/impl/VersionImpl.cpp
)

# Link the shared library to the test executable
target_link_libraries(test_${PROJECT_NAME} PRIVATE
    ${PROJECT_NAME}
    gtest_main
    SQLiteCpp
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    hnswlib
    zip
    jwt-cpp
)

# Include GoogleTest
include(GoogleTest)
gtest_discover_tests(test_${PROJECT_NAME})

# Enable testing
enable_testing()
add_test(NAME test_${PROJECT_NAME} COMMAND test_${PROJECT_NAME})
