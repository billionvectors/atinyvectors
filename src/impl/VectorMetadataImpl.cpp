#include "VectorMetadata.hpp"
#include "DatabaseManager.hpp"

namespace atinyvectors {

namespace {
void bindVectorMetadataParameters(SQLite::Statement& query, const VectorMetadata& metadata) {
    query.bind(1, metadata.vectorId);
    query.bind(2, metadata.key);
    query.bind(3, metadata.value);
}

VectorMetadata createVectorMetadataFromQuery(SQLite::Statement& query) {
    return VectorMetadata(
        query.getColumn(0).getInt64(),            // id
        query.getColumn(1).getInt64(),         // vectorId
        query.getColumn(2).getString(),         // key
        query.getColumn(3).getString()          // value
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
    createTable();
}

void VectorMetadataManager::createTable() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    db.exec("CREATE TABLE IF NOT EXISTS VectorMetadata ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "vectorId INTEGER NOT NULL, "
            "key TEXT NOT NULL, "
            "value TEXT);");
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
    SQLite::Statement insertQuery(db, "INSERT INTO VectorMetadata (vectorId, key, value) VALUES (?, ?, ?)");
    bindVectorMetadataParameters(insertQuery, metadata);
    insertQuery.exec();

    long insertedId = static_cast<long>(db.getLastInsertRowid());
    metadata.id = insertedId;

    transaction.commit();

    return insertedId;
}

std::vector<VectorMetadata> VectorMetadataManager::getAllVectorMetadata() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, vectorId, key, value FROM VectorMetadata");
    return executeSelectQuery(query);
}

VectorMetadata VectorMetadataManager::getVectorMetadataById(long id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, vectorId, key, value FROM VectorMetadata WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return createVectorMetadataFromQuery(query);
    }

    throw std::runtime_error("VectorMetadata not found");
}

std::vector<VectorMetadata> VectorMetadataManager::getVectorMetadataByVectorId(long vectorId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, vectorId, key, value FROM VectorMetadata WHERE vectorId = ?");
    query.bind(1, vectorId);

    return executeSelectQuery(query);
}

void VectorMetadataManager::updateVectorMetadata(const VectorMetadata& metadata) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    
    SQLite::Statement query(db, "UPDATE VectorMetadata SET vectorId = ?, key = ?, value = ? WHERE id = ?");
    bindVectorMetadataParameters(query, metadata);
    query.bind(4, metadata.id);
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

} // namespace atinyvectors
