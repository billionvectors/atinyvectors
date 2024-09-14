#include "VectorIndex.hpp"
#include "DatabaseManager.hpp"

namespace atinyvectors {

namespace {
void bindVectorIndexParameters(SQLite::Statement& query, const VectorIndex& vectorIndex) {
    query.bind(1, vectorIndex.versionId);
    query.bind(2, static_cast<int>(vectorIndex.vectorValueType));
    query.bind(3, vectorIndex.name);
    query.bind(4, vectorIndex.create_date_utc);
    query.bind(5, vectorIndex.updated_time_utc);
    query.bind(6, vectorIndex.is_default ? 1 : 0);
}

VectorIndex createVectorIndexFromQuery(SQLite::Statement& query) {
    return VectorIndex(
        query.getColumn(0).getInt(),         // id
        query.getColumn(1).getInt(),         // versionId
        static_cast<VectorValueType>(query.getColumn(2).getInt()),  // vectorValueType
        query.getColumn(3).getString(),      // name
        query.getColumn(4).getInt64(),       // create_date_utc
        query.getColumn(5).getInt64(),       // updated_time_utc
        query.getColumn(6).getInt() == 1     // is_default
    );
}

std::vector<VectorIndex> executeSelectQuery(SQLite::Statement& query) {
    std::vector<VectorIndex> vectorIndices;
    while (query.executeStep()) {
        vectorIndices.push_back(createVectorIndexFromQuery(query));
    }
    return vectorIndices;
}
} // anonymous namespace

std::unique_ptr<VectorIndexManager> VectorIndexManager::instance;
std::mutex VectorIndexManager::instanceMutex;

VectorIndexManager::VectorIndexManager() {
    createTable();
}

void VectorIndexManager::createTable() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    db.exec("CREATE TABLE IF NOT EXISTS VectorIndex ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "versionId INTEGER NOT NULL, "
            "vectorValueType INTEGER, "
            "name TEXT NOT NULL, "
            "create_date_utc INTEGER, "
            "updated_time_utc INTEGER, "
            "is_default BOOLEAN DEFAULT 0, "
            "FOREIGN KEY(versionId) REFERENCES Version(id));");
}

VectorIndexManager& VectorIndexManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new VectorIndexManager());
    }
    return *instance;
}

int VectorIndexManager::addVectorIndex(VectorIndex& vectorIndex) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
        
    if (vectorIndex.is_default) {
        SQLite::Statement updateQuery(db, "UPDATE VectorIndex SET is_default = 0 WHERE versionId = ?");
        updateQuery.bind(1, vectorIndex.versionId);
        updateQuery.exec();
    }

    SQLite::Statement insertQuery(db, "INSERT INTO VectorIndex (versionId, vectorValueType, name, create_date_utc, updated_time_utc, is_default) VALUES (?, ?, ?, ?, ?, ?)");
    bindVectorIndexParameters(insertQuery, vectorIndex);
    insertQuery.exec();

    int insertedId = static_cast<int>(db.getLastInsertRowid());
    vectorIndex.id = insertedId;

    transaction.commit();

    return insertedId;
}

std::vector<VectorIndex> VectorIndexManager::getAllVectorIndices() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, versionId, vectorValueType, name, create_date_utc, updated_time_utc, is_default FROM VectorIndex");
    return executeSelectQuery(query);
}

VectorIndex VectorIndexManager::getVectorIndexById(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, versionId, vectorValueType, name, create_date_utc, updated_time_utc, is_default FROM VectorIndex WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return createVectorIndexFromQuery(query);
    }

    throw std::runtime_error("VectorIndex not found");
}

std::vector<VectorIndex> VectorIndexManager::getVectorIndicesByVersionId(int versionId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, versionId, vectorValueType, name, create_date_utc, updated_time_utc, is_default FROM VectorIndex WHERE versionId = ?");
    query.bind(1, versionId);

    return executeSelectQuery(query);
}

void VectorIndexManager::updateVectorIndex(const VectorIndex& vectorIndex) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    
    if (vectorIndex.is_default) {
        SQLite::Statement updateQuery(db, "UPDATE VectorIndex SET is_default = 0 WHERE versionId = ? AND id != ?");
        updateQuery.bind(1, vectorIndex.versionId);
        updateQuery.bind(2, vectorIndex.id);
        updateQuery.exec();
    }

    SQLite::Statement query(db, "UPDATE VectorIndex SET versionId = ?, vectorValueType = ?, name = ?, create_date_utc = ?, updated_time_utc = ?, is_default = ? WHERE id = ?");
    query.bind(1, vectorIndex.versionId);
    query.bind(2, static_cast<int>(vectorIndex.vectorValueType));
    query.bind(3, vectorIndex.name);
    query.bind(4, vectorIndex.create_date_utc);
    query.bind(5, vectorIndex.updated_time_utc);
    query.bind(6, vectorIndex.is_default ? 1 : 0);
    query.bind(7, vectorIndex.id);
    query.exec();
    
    transaction.commit();
}

void VectorIndexManager::deleteVectorIndex(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);

    SQLite::Statement checkQuery(db, "SELECT versionId, is_default FROM VectorIndex WHERE id = ?");
    checkQuery.bind(1, id);
    
    int versionId = -1;
    bool isDefault = false;

    if (checkQuery.executeStep()) {
        versionId = checkQuery.getColumn(0).getInt();
        isDefault = checkQuery.getColumn(1).getInt() == 1;
    }

    SQLite::Statement deleteQuery(db, "DELETE FROM VectorIndex WHERE id = ?");
    deleteQuery.bind(1, id);
    deleteQuery.exec();

    if (isDefault) {
        SQLite::Statement findRecentQuery(db, "SELECT id FROM VectorIndex WHERE versionId = ? ORDER BY create_date_utc DESC LIMIT 1");
        findRecentQuery.bind(1, versionId);
        
        if (findRecentQuery.executeStep()) {
            int recentId = findRecentQuery.getColumn(0).getInt();
            SQLite::Statement setDefaultQuery(db, "UPDATE VectorIndex SET is_default = 1 WHERE id = ?");
            setDefaultQuery.bind(1, recentId);
            setDefaultQuery.exec();
        }
    }

    transaction.commit();
}

} // namespace atinyvectors
