cmake_minimum_required(VERSION 3.14)

project(atinyvectors VERSION 0.1.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure position-independent code is enabled for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# Fetch FAISS library
FetchContent_Declare(
  faiss
  GIT_REPOSITORY https://github.com/billionvectors/faiss_atinyvectors.git
  GIT_TAG v1.9.0-atv
)
set(FAISS_OPT_LEVEL "generic" CACHE STRING "Optimization level for Faiss")
set(FAISS_ENABLE_GPU OFF CACHE BOOL "Enable support for GPU indexes")
set(FAISS_ENABLE_RAFT OFF CACHE BOOL "Enable RAFT for GPU indexes")
set(FAISS_ENABLE_ROCM OFF CACHE BOOL "Enable ROCm for GPU indexes")
set(FAISS_ENABLE_PYTHON OFF CACHE BOOL "Build Python extension")
set(FAISS_ENABLE_C_API ON CACHE BOOL "Build C API")
set(FAISS_USE_LTO OFF CACHE BOOL "Enable Link-Time Optimization")
FetchContent_MakeAvailable(faiss)

# Fetch googletest library (googletest in faiss)
#FetchContent_Declare(
#  googletest
#  GIT_REPOSITORY https://github.com/google/googletest.git
#  GIT_TAG v1.15.2
#)
#FetchContent_MakeAvailable(googletest)

# Fetch SQLiteCpp library
FetchContent_Declare(
  SQLiteCpp
  GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
  GIT_TAG 3.3.2
)
FetchContent_MakeAvailable(SQLiteCpp)

# Fetch spdlog library
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.11.0
)
FetchContent_MakeAvailable(spdlog)

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

# Antlr4
FetchContent_Declare(
  antlr4-runtime
  GIT_REPOSITORY https://github.com/antlr/antlr4.git
  GIT_TAG 4.13.2
)
FetchContent_GetProperties(antlr4-runtime)

if(NOT antlr4-runtime_POPULATED)
  FetchContent_Populate(antlr4-runtime)
  add_subdirectory(${antlr4-runtime_SOURCE_DIR}/runtime/Cpp ${antlr4-runtime_BINARY_DIR}/runtime/Cpp)
endif()
include_directories(${antlr4-runtime_SOURCE_DIR}/runtime/Cpp/runtime/src)

# OpenSSL
find_package(OpenSSL REQUIRED)

# Include directories for header files
include_directories(parser)
include_directories(capi)
include_directories(include)
include_directories(src)

# Set compiler flags based on build type
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Build the library as a SHARED library
add_library(${PROJECT_NAME} SHARED
  capi/atinyvectors_c_api.cpp
  capi/atinyvectors_idcache.cpp
  capi/atinyvectors_rbac_c_api.cpp
  capi/atinyvectors_search_c_api.cpp
  capi/atinyvectors_snapshot_c_api.cpp
  capi/atinyvectors_space_c_api.cpp
  capi/atinyvectors_vector_c_api.cpp
  capi/atinyvectors_version_c_api.cpp

  src/impl/algo/FaissIndexManagerImpl.cpp
  src/impl/algo/FaissIndexLRUCacheImpl.cpp
  
  src/impl/filter/FilterManager.cpp
  src/impl/filter/SQLBuilderVisitor.cpp

  src/impl/service/RbacTokenServiceImpl.cpp 
  src/impl/service/SearchServiceImpl.cpp 
  src/impl/service/SnapshotServiceImpl.cpp 
  src/impl/service/SpaceServiceImpl.cpp 
  src/impl/service/VectorServiceImpl.cpp 
  src/impl/service/VersionServiceImpl.cpp 

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

  # antlr4
  parser/PlanLexer.cpp
  parser/PlanParser.cpp
  parser/PlanBaseVisitor.cpp
  parser/PlanVisitor.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SQLiteCpp
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    zip
    jwt-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
    antlr4_static
    faiss
)

target_compile_options(${PROJECT_NAME} PRIVATE ${disabled_compile_warnings})

# Add executable for the test program
add_executable(test_${PROJECT_NAME}
  tests/algo/FaissIndexManagerTest.cpp
  tests/algo/FaissIndexLRUCacheTest.cpp
  
  tests/filter/FilterManagerTest.cpp
  tests/filter/SQLBuilderVisitorTest.cpp

  tests/service/RbacTokenServiceTest.cpp
  tests/service/SearchServiceTest.cpp
  tests/service/SnapshotServiceTest.cpp
  tests/service/SpaceServiceTest.cpp
  tests/service/VectorServiceTest.cpp
  tests/service/VersionServiceTest.cpp

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
  capi/atinyvectors_idcache.cpp
  capi/atinyvectors_rbac_c_api.cpp
  capi/atinyvectors_search_c_api.cpp
  capi/atinyvectors_snapshot_c_api.cpp
  capi/atinyvectors_space_c_api.cpp
  capi/atinyvectors_vector_c_api.cpp
  capi/atinyvectors_version_c_api.cpp

  src/impl/algo/FaissIndexManagerImpl.cpp
  src/impl/algo/FaissIndexLRUCacheImpl.cpp

  src/impl/service/RbacTokenServiceImpl.cpp 
  src/impl/service/SearchServiceImpl.cpp 
  src/impl/service/SnapshotServiceImpl.cpp 
  src/impl/service/SpaceServiceImpl.cpp 
  src/impl/service/VectorServiceImpl.cpp 
  src/impl/service/VersionServiceImpl.cpp 

  src/impl/utils/UtilsImpl.cpp
  
  src/impl/filter/FilterManager.cpp
  src/impl/filter/SQLBuilderVisitor.cpp

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

  # antlr4
  parser/PlanLexer.cpp
  parser/PlanParser.cpp
  parser/PlanBaseVisitor.cpp
  parser/PlanVisitor.cpp
)

# Link the shared library to the test executable
target_link_libraries(test_${PROJECT_NAME} PRIVATE
    ${PROJECT_NAME}
    gtest_main
    SQLiteCpp
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    zip
    jwt-cpp
    antlr4_static
    faiss
)

# Include GoogleTest
include(GoogleTest)
gtest_discover_tests(test_${PROJECT_NAME})

# Enable testing
enable_testing()
add_test(NAME test_${PROJECT_NAME} COMMAND test_${PROJECT_NAME})
