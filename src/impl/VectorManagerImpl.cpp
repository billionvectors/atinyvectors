// VectorManagerImpl.cpp
#include "Vector.hpp"
#include "VectorIndex.hpp"
#include "VectorIndexOptimizer.hpp"
#include "DatabaseManager.hpp"
#include "algo/HnswIndexLRUCache.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <sstream>
#include <cstring>
#include "spdlog/spdlog.h"
#include "Config.hpp"

using namespace atinyvectors;
using namespace atinyvectors::algo;

std::unique_ptr<VectorManager> VectorManager::instance;
std::mutex VectorManager::instanceMutex;

VectorManager::VectorManager() {
    createTable();
}

void VectorManager::createTable() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    db.exec("CREATE TABLE IF NOT EXISTS Vector ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "versionId INTEGER NOT NULL, "
            "unique_id INTEGER NOT NULL, "
            "type INTEGER, "
            "deleted BOOLEAN DEFAULT 0);");
    db.exec("CREATE TABLE IF NOT EXISTS VectorValue ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "vectorId INTEGER NOT NULL,"
            "vectorIndexId INTEGER NOT NULL,"
            "type INTEGER,"
            "data BLOB,"
            "FOREIGN KEY(vectorId) REFERENCES Vector(id),"
            "FOREIGN KEY(vectorIndexId) REFERENCES VectorIndex(id));");
}

VectorManager& VectorManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new VectorManager());
    }
    return *instance;
}

int VectorManager::addVector(Vector& vector) {
    // create table if necessary
    VectorIndexManager::getInstance();
    VectorIndexOptimizerManager::getInstance();

    auto& db = DatabaseManager::getInstance().getDatabase();
    spdlog::info("Starting transaction for adding/updating vector with UniqueID: {}, VersionID: {}", vector.unique_id, vector.versionId);
    SQLite::Transaction transaction(db);

    try {
        if (vector.unique_id > 0) {
            spdlog::debug("Checking if vector with UniqueID: {} and VersionID: {} already exists", vector.unique_id, vector.versionId);
            SQLite::Statement checkQuery(db, "SELECT id FROM Vector WHERE versionId = ? AND unique_id = ?");
            checkQuery.bind(1, vector.versionId);
            checkQuery.bind(2, vector.unique_id);

            if (checkQuery.executeStep()) {
                vector.id = checkQuery.getColumn(0).getInt64();
                spdlog::info("Vector with UniqueID {} exists. Updating vector ID: {}", vector.unique_id, vector.id);

                spdlog::debug("Updating Vector table for vector ID: {}", vector.id);
                SQLite::Statement updateQuery(db, "UPDATE Vector SET versionId = ?, unique_id = ?, type = ?, deleted = ? WHERE id = ?");
                updateQuery.bind(1, vector.versionId);
                updateQuery.bind(2, vector.unique_id);
                updateQuery.bind(3, static_cast<int>(vector.type));
                updateQuery.bind(4, vector.deleted ? 1 : 0);
                updateQuery.bind(5, static_cast<int>(vector.id));
                updateQuery.exec();

                spdlog::debug("Deleting old VectorValue entries for vector ID: {}", vector.id);
                SQLite::Statement deleteValueQuery(db, "DELETE FROM VectorValue WHERE vectorId = ?");
                deleteValueQuery.bind(1, static_cast<int>(vector.id));
                deleteValueQuery.exec();
            } else {
                spdlog::info("Vector with UniqueID {} does not exist. Inserting new vector.", vector.unique_id);

                SQLite::Statement query(db, "INSERT INTO Vector (versionId, unique_id, type, deleted) VALUES (?, ?, ?, ?)");
                query.bind(1, vector.versionId);
                query.bind(2, vector.unique_id);
                query.bind(3, static_cast<int>(vector.type));
                query.bind(4, vector.deleted ? 1 : 0);
                query.exec();

                vector.id = static_cast<int>(db.getLastInsertRowid());
                spdlog::debug("Inserted new vector with auto-assigned ID: {}", vector.id);
            }
        } else {
            spdlog::info("Inserting new vector without UniqueID for VersionID: {}", vector.versionId);

            SQLite::Statement maxUniqueIdQuery(db, "SELECT IFNULL(MAX(unique_id), 0) + 1 FROM Vector WHERE versionId = ?");
            maxUniqueIdQuery.bind(1, vector.versionId);
            maxUniqueIdQuery.executeStep();
            vector.unique_id = maxUniqueIdQuery.getColumn(0).getInt();

            SQLite::Statement query(db, "INSERT INTO Vector (versionId, unique_id, type, deleted) VALUES (?, ?, ?, ?)");
            query.bind(1, vector.versionId);
            query.bind(2, vector.unique_id);
            query.bind(3, static_cast<int>(vector.type));
            query.bind(4, vector.deleted ? 1 : 0);
            query.exec();

            vector.id = static_cast<int>(db.getLastInsertRowid());
            spdlog::debug("Inserted new vector with auto-assigned ID: {}", vector.id);
        }

        spdlog::info("Processing VectorValue entries for vector ID: {}", vector.id);

        // Cache for HNSW configurations to avoid redundant queries
        std::unordered_map<int, nlohmann::json> hnswConfigCache;

        for (auto& value : vector.values) {
            spdlog::debug("Inserting VectorValue for vector ID: {}, vectorIndexId: {}", vector.id, value.vectorIndexId);

            SQLite::Statement valueQuery(db, "INSERT INTO VectorValue (vectorId, vectorIndexId, type, data) VALUES (?, ?, ?, ?)");
            valueQuery.bind(1, vector.id);
            valueQuery.bind(2, value.vectorIndexId);
            valueQuery.bind(3, static_cast<int>(value.type));
            std::vector<uint8_t> serializedData = value.serialize();
            valueQuery.bind(4, serializedData.data(), static_cast<int>(serializedData.size()));
            valueQuery.exec();

            value.id = static_cast<int>(db.getLastInsertRowid());
            spdlog::debug("Inserted VectorValue with ID: {} for vector ID: {}", value.id, vector.id);

            // Process based on vector type
            if (value.type == VectorValueType::Dense || value.type == VectorValueType::Sparse || value.type == VectorValueType::MultiVector) {
                spdlog::info("Processing HNSW index update for vector ID: {}", vector.id);
                
                std::string indexFileName = std::to_string(vector.versionId) + "-" + std::to_string(vector.versionId) + "-" + std::to_string(value.vectorIndexId) + ".idx";

                // Check if the HNSW config for the vectorIndexId is already cached
                if (hnswConfigCache.find(value.vectorIndexId) == hnswConfigCache.end()) {
                    // Fetch HNSW config only if not already cached
                    SQLite::Statement optimizerQuery(db, "SELECT hnswConfigJson FROM VectorIndexOptimizer WHERE vectorIndexId = ?");
                    optimizerQuery.bind(1, value.vectorIndexId);

                    if (optimizerQuery.executeStep()) {
                        std::string hnswConfigJson = optimizerQuery.getColumn(0).getString();
                        nlohmann::json hnswConfig = nlohmann::json::parse(hnswConfigJson);
                        hnswConfigCache[value.vectorIndexId] = hnswConfig;  // Cache the result
                    } else {
                        spdlog::error("Failed to fetch VectorIndexOptimizer for vectorIndexId: {}", value.vectorIndexId);
                        throw std::runtime_error("Failed to fetch VectorIndexOptimizer for vectorIndexId: " + std::to_string(value.vectorIndexId));
                    }
                }

                // Use the cached configuration
                const nlohmann::json& hnswConfig = hnswConfigCache[value.vectorIndexId];
                int M = hnswConfig.value("M", 16);
                int efConstruction = hnswConfig.value("EfConstruct", 200);

                spdlog::debug("HNSW configuration: M={}, EfConstruct={}", M, efConstruction);

                // Get HnswIndexManager and add vector data
                auto hnswManager = HnswIndexLRUCache::getInstance().get(value.vectorIndexId, indexFileName, 128, 10000);
                
                // Add vector data based on type
                if (value.type == VectorValueType::Dense) {
                    hnswManager->addVectorData(value.denseData, vector.unique_id);
                } else if (value.type == VectorValueType::Sparse) {
                    hnswManager->addVectorData(value.sparseValues, vector.unique_id);  // Sparse data handling
                } else if (value.type == VectorValueType::MultiVector) {
                    for (const auto& mvData : value.multiVectorData) {
                        hnswManager->addVectorData(mvData, vector.unique_id);  // MultiVector data handling
                    }
                }
            }
        }

        transaction.commit();
        spdlog::info("Transaction committed successfully for Vector UniqueID: {}", vector.unique_id);
    } catch (const std::exception& e) {
        spdlog::error("Exception occurred while adding or updating vector: {}", e.what());
        transaction.rollback();
        throw;
    }

    return vector.id;
}


std::vector<Vector> VectorManager::getAllVectors() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, versionId, unique_id, type, deleted FROM Vector");
    
    std::vector<Vector> vectors;
    while (query.executeStep()) {
        Vector vector;
        vector.id = query.getColumn(0).getInt64();
        vector.versionId = query.getColumn(1).getInt();
        vector.unique_id = query.getColumn(2).getInt();
        vector.type = static_cast<VectorValueType>(query.getColumn(3).getInt());
        vector.deleted = query.getColumn(4).getInt() != 0;

        SQLite::Statement valueQuery(db, "SELECT id, vectorId, vectorIndexId, type, data FROM VectorValue WHERE vectorId = ?");
        valueQuery.bind(1, vector.id);
        
        while (valueQuery.executeStep()) {
            VectorValue value;
            value.id = valueQuery.getColumn(0).getInt();
            value.vectorId = valueQuery.getColumn(1).getInt64();
            value.vectorIndexId = valueQuery.getColumn(2).getInt();
            value.type = static_cast<VectorValueType>(valueQuery.getColumn(3).getInt());
            std::vector<uint8_t> blobData;
            blobData.assign(static_cast<const uint8_t*>(valueQuery.getColumn(4).getBlob()), 
                            static_cast<const uint8_t*>(valueQuery.getColumn(4).getBlob()) + valueQuery.getColumn(4).getBytes());
            value.deserialize(blobData);
            vector.values.push_back(value);
        }

        vectors.push_back(vector);
    }

    return vectors;
}

Vector VectorManager::getVectorById(unsigned long long id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, versionId, unique_id, type, deleted FROM Vector WHERE id = ?");
    query.bind(1, static_cast<int>(id));

    if (query.executeStep()) {
        Vector vector;
        vector.id = query.getColumn(0).getInt64();
        vector.versionId = query.getColumn(1).getInt();
        vector.unique_id = query.getColumn(2).getInt();
        vector.type = static_cast<VectorValueType>(query.getColumn(3).getInt());
        vector.deleted = query.getColumn(4).getInt() != 0;

        // Retrieve associated VectorValues
        SQLite::Statement valueQuery(db, "SELECT id, vectorId, vectorIndexId, type, data FROM VectorValue WHERE vectorId = ?");
        valueQuery.bind(1, vector.id);
        
        while (valueQuery.executeStep()) {
            VectorValue value;
            value.id = valueQuery.getColumn(0).getInt();
            value.vectorId = valueQuery.getColumn(1).getInt64();
            value.vectorIndexId = valueQuery.getColumn(2).getInt();
            value.type = static_cast<VectorValueType>(valueQuery.getColumn(3).getInt());

            std::vector<uint8_t> blobData;
            blobData.assign(static_cast<const uint8_t*>(valueQuery.getColumn(4).getBlob()), 
                            static_cast<const uint8_t*>(valueQuery.getColumn(4).getBlob()) + valueQuery.getColumn(4).getBytes());

            // Deserialize the data into VectorValue
            value.deserialize(blobData);
            vector.values.push_back(value);
        }

        return vector;
    }

    throw std::runtime_error("Vector not found");
}

Vector VectorManager::getVectorByUniqueId(int versionId, int unique_id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, versionId, unique_id, type, deleted FROM Vector WHERE versionId = ? AND unique_id = ?");
    query.bind(1, versionId);
    query.bind(2, unique_id);

    if (query.executeStep()) {
        Vector vector;
        vector.id = query.getColumn(0).getInt64();
        vector.versionId = query.getColumn(1).getInt();
        vector.unique_id = query.getColumn(2).getInt();
        vector.type = static_cast<VectorValueType>(query.getColumn(3).getInt());
        vector.deleted = query.getColumn(4).getInt() != 0;

        // Fetch associated VectorValues
        SQLite::Statement valueQuery(db, "SELECT id, vectorId, vectorIndexId, type, data FROM VectorValue WHERE vectorId = ?");
        valueQuery.bind(1, vector.id);

        while (valueQuery.executeStep()) {
            VectorValue value;
            value.id = valueQuery.getColumn(0).getInt();
            value.vectorId = valueQuery.getColumn(1).getInt64();
            value.vectorIndexId = valueQuery.getColumn(2).getInt();
            value.type = static_cast<VectorValueType>(valueQuery.getColumn(3).getInt());
            std::vector<uint8_t> blobData;
            blobData.assign(static_cast<const uint8_t*>(valueQuery.getColumn(4).getBlob()), 
                            static_cast<const uint8_t*>(valueQuery.getColumn(4).getBlob()) + valueQuery.getColumn(4).getBytes());

            value.deserialize(blobData);
            vector.values.push_back(value);
        }

        return vector;
    }

    throw std::runtime_error("Vector not found with the specified versionId and unique_id.");
}

std::vector<Vector> VectorManager::getVectorsByVersionId(int versionId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, versionId, unique_id, type, deleted FROM Vector WHERE versionId = ?");
    query.bind(1, versionId);
    
    std::vector<Vector> vectors;
    spdlog::info("Fetching vectors for versionId: {}", versionId);  // Replaced cout with spdlog

    while (query.executeStep()) {
        Vector vector;
        vector.id = query.getColumn(0).getInt64();
        vector.versionId = query.getColumn(1).getInt();
        vector.unique_id = query.getColumn(2).getInt(); // Fetch the unique_id
        vector.type = static_cast<VectorValueType>(query.getColumn(3).getInt());
        vector.deleted = query.getColumn(4).getInt() != 0;

        spdlog::debug("Fetched vector - ID: {}, VersionID: {}, UniqueID: {}, Type: {}, Deleted: {}", 
                      vector.id, vector.versionId, vector.unique_id, static_cast<int>(vector.type), vector.deleted);

        // Retrieve associated VectorValues
        SQLite::Statement valueQuery(db, "SELECT id, vectorId, vectorIndexId, type, data FROM VectorValue WHERE vectorId = ?");
        valueQuery.bind(1, vector.id);

        while (valueQuery.executeStep()) {
            VectorValue value;
            value.id = valueQuery.getColumn(0).getInt();
            value.vectorId = valueQuery.getColumn(1).getInt64();
            value.vectorIndexId = valueQuery.getColumn(2).getInt();
            value.type = static_cast<VectorValueType>(valueQuery.getColumn(3).getInt());

            std::vector<uint8_t> blobData;
            blobData.assign(static_cast<const uint8_t*>(valueQuery.getColumn(4).getBlob()),
                            static_cast<const uint8_t*>(valueQuery.getColumn(4).getBlob()) + valueQuery.getColumn(4).getBytes());

            spdlog::debug("Processing VectorValue - ID: {}, VectorID: {}, Index: {}, Type: {}", 
                          value.id, value.vectorId, value.vectorIndexId, static_cast<int>(value.type));

            // Deserialize the data into VectorValue
            value.deserialize(blobData);
            vector.values.push_back(value);
        }

        vectors.push_back(vector);
        spdlog::debug("Added vector to list - ID: {}", vector.id); // Log when a vector is added to the list
    }

    spdlog::info("Finished fetching vectors for versionId: {}. Total vectors fetched: {}", versionId, vectors.size());  // Log total fetched vectors

    return vectors;
}

void VectorManager::updateVector(const Vector& vector) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Transaction transaction(db);
    SQLite::Statement query(db, "UPDATE Vector SET versionId = ?, type = ?, deleted = ? WHERE id = ?");
    query.bind(1, vector.versionId);
    query.bind(2, static_cast<int>(vector.type));
    query.bind(3, vector.deleted ? 1 : 0);
    query.bind(4, static_cast<int>(vector.id));
    query.exec();

    SQLite::Statement deleteValueQuery(db, "DELETE FROM VectorValue WHERE vectorId = ?");
    deleteValueQuery.bind(1, static_cast<int>(vector.id));
    deleteValueQuery.exec();

    for (auto& value : vector.values) {
        SQLite::Statement valueQuery(db, "INSERT INTO VectorValue (vectorId, vectorIndexId, type, data) VALUES (?, ?, ?, ?)");
        valueQuery.bind(1, vector.id);
        valueQuery.bind(2, value.vectorIndexId);
        valueQuery.bind(3, static_cast<int>(value.type));
        std::vector<uint8_t> serializedData = value.serialize();
        valueQuery.bind(4, serializedData.data(), static_cast<int>(serializedData.size()));
        valueQuery.exec();
    }

    transaction.commit();
}

void VectorManager::deleteVector(unsigned long long id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Transaction transaction(db);
    SQLite::Statement query(db, "DELETE FROM Vector WHERE id = ?");
    query.bind(1, static_cast<int>(id));
    query.exec();

    SQLite::Statement deleteValueQuery(db, "DELETE FROM VectorValue WHERE vectorId = ?");
    deleteValueQuery.bind(1, static_cast<int>(id));
    deleteValueQuery.exec();

    transaction.commit();
}