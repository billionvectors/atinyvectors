#include <unordered_set>
#include <sstream>

#include "VectorMetadata.hpp"
#include "DatabaseManager.hpp"
#include "filter/FilterManager.hpp"
#include "spdlog/spdlog.h"

using namespace atinyvectors::filter;

namespace atinyvectors {

namespace {
    
void bindVectorMetadataParameters(SQLite::Statement& query, const VectorMetadata& metadata) {
    query.bind(1, metadata.vectorId);
    query.bind(2, metadata.key);
    query.bind(3, metadata.value);
    query.bind(4, metadata.versionId);
}

VectorMetadata createVectorMetadataFromQuery(SQLite::Statement& query) {
    return VectorMetadata(
        query.getColumn(0).getInt64(),  // id
        query.getColumn(4).getInt64(),   // versionId
        query.getColumn(1).getInt64(),  // vectorId
        query.getColumn(2).getString(), // key
        query.getColumn(3).getString() // value
    );
}

std::vector<VectorMetadata> executeSelectQuery(SQLite::Statement& query) {
    std::vector<VectorMetadata> metadataList;
    while (query.executeStep()) {
        metadataList.push_back(createVectorMetadataFromQuery(query));
    }
    return metadataList;
}

} // anonymous namespace

std::unique_ptr<VectorMetadataManager> VectorMetadataManager::instance;
std::mutex VectorMetadataManager::instanceMutex;

VectorMetadataManager::VectorMetadataManager() {
}

VectorMetadataManager& VectorMetadataManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new VectorMetadataManager());
    }
    return *instance;
}

long VectorMetadataManager::addVectorMetadata(VectorMetadata& metadata) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    SQLite::Statement insertQuery(db, "INSERT INTO VectorMetadata (vectorId, key, value, versionId) VALUES (?, ?, ?, ?)");
    bindVectorMetadataParameters(insertQuery, metadata);
    insertQuery.exec();

    long insertedId = static_cast<long>(db.getLastInsertRowid());
    metadata.id = insertedId;

    transaction.commit();

    return insertedId;
}

std::vector<VectorMetadata> VectorMetadataManager::getAllVectorMetadata() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, vectorId, key, value, versionId FROM VectorMetadata");
    return executeSelectQuery(query);
}

VectorMetadata VectorMetadataManager::getVectorMetadataById(long id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, vectorId, key, value, versionId FROM VectorMetadata WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return createVectorMetadataFromQuery(query);
    }

    throw std::runtime_error("VectorMetadata not found");
}

std::vector<VectorMetadata> VectorMetadataManager::getVectorMetadataByVectorId(long vectorId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, vectorId, key, value, versionId FROM VectorMetadata WHERE vectorId = ?");
    query.bind(1, vectorId);

    return executeSelectQuery(query);
}

void VectorMetadataManager::updateVectorMetadata(const VectorMetadata& metadata) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    
    SQLite::Statement query(db, "UPDATE VectorMetadata SET vectorId = ?, key = ?, value = ?, versionId = ? WHERE id = ?");
    bindVectorMetadataParameters(query, metadata);
    query.bind(5, metadata.id);
    query.exec();
    
    transaction.commit();
}

void VectorMetadataManager::deleteVectorMetadata(long id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    
    SQLite::Statement query(db, "DELETE FROM VectorMetadata WHERE id = ?");
    query.bind(1, id);
    query.exec();

    transaction.commit();
}

void VectorMetadataManager::deleteVectorMetadataByVectorId(long vectorId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    
    SQLite::Statement query(db, "DELETE FROM VectorMetadata WHERE vectorId = ?");
    query.bind(1, vectorId);
    query.exec();

    transaction.commit();
}

std::vector<std::pair<float, int>> VectorMetadataManager::filterVectors(
    const std::vector<std::pair<float, int>>& inputVectors,
    const std::string& filter) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    std::vector<std::pair<float, int>> filteredVectors;
    std::string sqlFilter = FilterManager::getInstance().toSQL(filter);

    std::stringstream uniqueIdListStream;
    uniqueIdListStream << "(";
    for (size_t i = 0; i < inputVectors.size(); ++i) {
        uniqueIdListStream << inputVectors[i].second;
        if (i < inputVectors.size() - 1) {
            uniqueIdListStream << ", ";
        }
    }
    uniqueIdListStream << ")";
    std::string uniqueIdList = uniqueIdListStream.str();

    std::string idQueryStr = "SELECT V.id, V.unique_id FROM Vector V WHERE V.unique_id IN " + uniqueIdList;
    SQLite::Statement idQuery(db, idQueryStr);

    std::unordered_map<long, long> uniqueIdToRealId;
    while (idQuery.executeStep()) {
        long id = idQuery.getColumn(0).getInt64();
        long uniqueId = idQuery.getColumn(1).getInt64();
        uniqueIdToRealId[uniqueId] = id;
    }

    std::stringstream idListStream;
    idListStream << "(";
    for (auto it = uniqueIdToRealId.begin(); it != uniqueIdToRealId.end(); ++it) {
        idListStream << it->second;
        if (std::next(it) != uniqueIdToRealId.end()) {
            idListStream << ", ";
        }
    }
    idListStream << ")";
    std::string idList = idListStream.str();

    std::string filterQueryStr = "SELECT VectorMetadata.vectorId FROM VectorMetadata WHERE VectorMetadata.vectorId IN " + idList + " AND " + sqlFilter;
    SQLite::Statement filterQuery(db, filterQueryStr);

    std::unordered_set<long> validRealIds;
    while (filterQuery.executeStep()) {
        long validId = filterQuery.getColumn(0).getInt64();
        validRealIds.insert(validId);
    }

    for (const auto& vec : inputVectors) {
        auto it = uniqueIdToRealId.find(vec.second);
        if (it != uniqueIdToRealId.end() && validRealIds.find(it->second) != validRealIds.end()) {
            filteredVectors.push_back(vec);
        }
    }

    return filteredVectors;
}

VectorMetadataResult VectorMetadataManager::queryVectors(
    long versionId, const std::string& filter, int start, int limit) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    VectorMetadataResult result;
    std::string sqlFilter = FilterManager::getInstance().toSQL(filter);

    // Query to get the total count of vectors matching the filter and versionId
    std::string countQueryStr = "SELECT COUNT(*) FROM VectorMetadata WHERE versionId = ? AND " + sqlFilter;
    SQLite::Statement countQuery(db, countQueryStr);
    countQuery.bind(1, versionId);
    if (countQuery.executeStep()) {
        result.totalCount = countQuery.getColumn(0).getInt();
    }

    // Query to get the filtered vectors with pagination and versionId
    std::string queryStr = "SELECT vectorId FROM VectorMetadata WHERE versionId = ? AND " + sqlFilter + 
                           " LIMIT ? OFFSET ?";
    SQLite::Statement query(db, queryStr);
    query.bind(1, versionId);
    query.bind(2, limit);
    query.bind(3, start);

    while (query.executeStep()) {
        int vectorId = query.getColumn(0).getInt();
        result.vectorUniqueIds.emplace_back(vectorId);
    }

    return result;
}

} // namespace atinyvectors
