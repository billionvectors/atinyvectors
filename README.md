# atinyvectors

atinyvectors is a high-performance vector search engine designed for efficient indexing and querying of large-scale vector data. It supports both dense and sparse vector types and allows users to manage spaces, versions, and vectors with customizable indexing configurations.

This project is a library project. If you want to obtain an executable binary, please refer to [asimplevectors](https://github.com/billionvectors/asimplevectors).

## Features

- **Efficient vector search** using HNSW (Hierarchical Navigable Small World) graph.
- Supports **dense and sparse** vector types.
- Customizable indexing configurations using **HNSW and Quantization**.
- Includes a C API for external integrations.

## Requirements

- Docker
- CMake 3.14+
- Clang (or other C++ compiler)
- GoogleTest for unit testing
- SQLiteCpp, libzip and nlohmann/json libraries

## Docs

For detailed development guidelines and documentation, please refer to the official guide at [https://docs.asimplevectors.com/](https://docs.asimplevectors.com/).

## Building the Project

The easiest way to build AtinyVectors is to use Docker.

### Build the Docker Image

To build the Docker image, simply run the following command:

```bash
./dockerbuild.sh
```
This script will build the project in Release mode by default. To build in Debug mode, you can modify the script or pass a debug option.

```bash
./dockerbuild.sh debug
```
Extract the Library
After building the project, you can extract the compiled libatinyvectors.so shared library using the Docker container:

### Running Tests
Unit tests are written using GoogleTest and can be run inside the Docker container. After building the project, you can run the tests as follows:

```bash
build_run_test.sh
```

### Usage
The project includes several Data Transfer Objects (DTOs) for managing spaces, versions, and vectors. You can use these DTOs via the provided C API or directly in C++.

Example Usage
The C++ API allows you to create spaces, add vectors, and perform searches. Here is an example of how to use the SearchDTO to perform a vector search:

```cpp
#include "dto/SearchDTO.hpp"

int main() {
    atinyvectors::dto::SearchDTOManager searchManager;
    std::string queryJson = "{\"vector\": [0.25, 0.45, 0.75]}";
    
    auto results = searchManager.search("space_name", queryJson, 5);
    
    for (const auto& result : results) {
        std::cout << "Distance: " << result.first << ", ID: " << result.second << std::endl;
    }

    return 0;
}
```

### C API
A C API is provided for integrating with Rust or other programming languages. You can link the shared library libatinyvectors.so and use the functions defined in atinyvectors_c_api.h to interact with the vector search engine.

### Contributing
Contributions are welcome! Please open an issue or submit a pull request if you would like to contribute to atinyvectors.

## License
For details, please refer to the `LICENSE` file.
