#include "VectorIndexOptimizer.hpp"
#include "DatabaseManager.hpp"

namespace atinyvectors {

namespace {
void bindOptimizerParameters(SQLite::Statement& query, const VectorIndexOptimizer& optimizer) {
    query.bind(1, optimizer.vectorIndexId);
    query.bind(2, static_cast<int>(optimizer.metricType));
    query.bind(3, optimizer.dimension);
    query.bind(4, optimizer.hnswConfigJson);
    query.bind(5, optimizer.quantizationConfigJson);
}

VectorIndexOptimizer createOptimizerFromQuery(SQLite::Statement& query) {
    return VectorIndexOptimizer(
        query.getColumn(0).getInt(),           // Id
        query.getColumn(1).getInt(),           // VectorIndexId
        static_cast<MetricType>(query.getColumn(2).getInt()), // MetricType
        query.getColumn(3).getInt(),           // Dimension
        query.getColumn(4).getString(),        // HnswConfigJson
        query.getColumn(5).getString()         // QuantizationConfigJson
    );
}

std::vector<VectorIndexOptimizer> executeSelectQuery(SQLite::Statement& query) {
    std::vector<VectorIndexOptimizer> optimizers;
    while (query.executeStep()) {
        optimizers.push_back(createOptimizerFromQuery(query));
    }
    return optimizers;
}
} // anonymous namespace

std::unique_ptr<VectorIndexOptimizerManager> VectorIndexOptimizerManager::instance;
std::mutex VectorIndexOptimizerManager::instanceMutex;

VectorIndexOptimizerManager::VectorIndexOptimizerManager() {
    createTable();
}

void VectorIndexOptimizerManager::createTable() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    db.exec("CREATE TABLE IF NOT EXISTS VectorIndexOptimizer ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "vectorIndexId INTEGER NOT NULL, "
            "metricType INTEGER, "
            "dimension INTEGER, "
            "hnswConfigJson TEXT, "
            "quantizationConfigJson TEXT);");
}

VectorIndexOptimizerManager& VectorIndexOptimizerManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new VectorIndexOptimizerManager());
    }
    return *instance;
}

int VectorIndexOptimizerManager::addOptimizer(VectorIndexOptimizer& optimizer) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    SQLite::Statement insertQuery(db, "INSERT INTO VectorIndexOptimizer (vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson) VALUES (?, ?, ?, ?, ?)");
    bindOptimizerParameters(insertQuery, optimizer);
    insertQuery.exec();

    int insertedId = static_cast<int>(db.getLastInsertRowid());
    optimizer.id = insertedId;

    transaction.commit();

    return insertedId;
}

std::vector<VectorIndexOptimizer> VectorIndexOptimizerManager::getAllOptimizers() {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson FROM VectorIndexOptimizer");
    return executeSelectQuery(query);
}

VectorIndexOptimizer VectorIndexOptimizerManager::getOptimizerById(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson FROM VectorIndexOptimizer WHERE id = ?");
    query.bind(1, id);

    if (query.executeStep()) {
        return createOptimizerFromQuery(query);
    }

    throw std::runtime_error("VectorIndexOptimizer not found");
}

std::vector<VectorIndexOptimizer> VectorIndexOptimizerManager::getOptimizerByIndexId(int vectorIndexId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT id, vectorIndexId, metricType, dimension, hnswConfigJson, quantizationConfigJson FROM VectorIndexOptimizer WHERE vectorIndexId = ?");
    query.bind(1, vectorIndexId);

    return executeSelectQuery(query);
}

void VectorIndexOptimizerManager::updateOptimizer(const VectorIndexOptimizer& optimizer) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    
    SQLite::Statement query(db, "UPDATE VectorIndexOptimizer SET vectorIndexId = ?, metricType = ?, dimension = ?, hnswConfigJson = ?, quantizationConfigJson = ? WHERE id = ?");
    bindOptimizerParameters(query, optimizer);
    query.bind(6, optimizer.id);
    query.exec();
    
    transaction.commit();
}

void VectorIndexOptimizerManager::deleteOptimizer(int id) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    
    SQLite::Transaction transaction(db);
    
    SQLite::Statement query(db, "DELETE FROM VectorIndexOptimizer WHERE id = ?");
    query.bind(1, id);
    query.exec();

    transaction.commit();
}

} // namespace atinyvectors
