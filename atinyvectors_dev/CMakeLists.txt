cmake_minimum_required(VERSION 3.14)

project(atinyvectors VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure position-independent code is enabled for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# Fetch SQLiteCpp library
FetchContent_Declare(
  SQLiteCpp
  GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
  GIT_TAG master
)
FetchContent_MakeAvailable(SQLiteCpp)

# Fetch GoogleTest library
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Fetch nlohmann/json library
FetchContent_Declare(json 
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

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
  GIT_TAG master
)
FetchContent_MakeAvailable(hnswlib)

# Include directories for header files
include_directories(capi)
include_directories(include)
include_directories(src)

# Set compiler flags based on build type
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Build the library as a SHARED library
add_library(${PROJECT_NAME} SHARED
  capi/atinyvectors_c_api.cpp

  src/impl/algo/HnswIndexLRUCacheImpl.cpp
  src/impl/algo/HnswIndexManagerImpl.cpp

  src/impl/dto/SearchDTOImpl.cpp 
  src/impl/dto/SpaceDTOImpl.cpp 
  src/impl/dto/VectorDTOImpl.cpp 
  src/impl/dto/VersionDTOImpl.cpp 
  
  src/impl/ConfigImpl.cpp 
  src/impl/DatabaseManagerImpl.cpp 
  src/impl/SpaceImpl.cpp
  src/impl/VectorIndexImpl.cpp
  src/impl/VectorIndexOptimizerImpl.cpp
  src/impl/VectorManagerImpl.cpp
  src/impl/VectorMetadataImpl.cpp
  src/impl/VectorValueImpl.cpp
  src/impl/VersionImpl.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE SQLiteCpp nlohmann_json::nlohmann_json spdlog::spdlog hnswlib)

# Add executable for the test program
add_executable(test_${PROJECT_NAME} 
  tests/algo/HnswIndexLRUCacheTest.cpp
  tests/algo/HnswIndexManagerTest.cpp
  tests/algo/HnswLibTest.cpp

  tests/capi/atinyvectors_c_api_spacedto_test.cpp

  tests/dto/SearchDTOTest.cpp
  tests/dto/SpaceDTOTest.cpp
  tests/dto/VectorDTOTest.cpp
  tests/dto/VersionDTOTest.cpp
  
  tests/ConfigTest.cpp 
  tests/SearchTest.cpp 
  tests/SpaceTest.cpp 
  tests/VectorIndexOptimizerTest.cpp
  tests/VectorIndexTest.cpp
  tests/VectorMetadataTest.cpp
  tests/VectorTest.cpp
  tests/VersionTest.cpp

  # Source files from the library
  capi/atinyvectors_c_api.cpp

  src/impl/algo/HnswIndexLRUCacheImpl.cpp
  src/impl/algo/HnswIndexManagerImpl.cpp

  src/impl/dto/SearchDTOImpl.cpp 
  src/impl/dto/SpaceDTOImpl.cpp 
  src/impl/dto/VectorDTOImpl.cpp 
  src/impl/dto/VersionDTOImpl.cpp 

  src/impl/ConfigImpl.cpp 
  src/impl/DatabaseManagerImpl.cpp 
  src/impl/SpaceImpl.cpp
  src/impl/VectorIndexImpl.cpp
  src/impl/VectorIndexOptimizerImpl.cpp
  src/impl/VectorManagerImpl.cpp
  src/impl/VectorMetadataImpl.cpp
  src/impl/VectorValueImpl.cpp
  src/impl/VersionImpl.cpp
)

# Link the shared library to the test executable
target_link_libraries(test_${PROJECT_NAME} PRIVATE ${PROJECT_NAME} gtest_main SQLiteCpp nlohmann_json::nlohmann_json spdlog::spdlog hnswlib)

# Include GoogleTest
include(GoogleTest)
gtest_discover_tests(test_${PROJECT_NAME})

# Enable testing
enable_testing()
add_test(NAME test_${PROJECT_NAME} COMMAND test_${PROJECT_NAME})
