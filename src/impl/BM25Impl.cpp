#include "BM25.hpp"
#include "DatabaseManager.hpp"
#include "spdlog/spdlog.h"

#include <cmath>
#include <sstream>
#include <algorithm>
#include <unordered_map>

namespace atinyvectors {

namespace {
float calculateBM25(
        float termFrequency, float docFrequency, float docLength,
        float avgDocLength, size_t totalDocs,
        float k1 = 1.5, float b = 0.75) {
    float idf = std::log((totalDocs - docFrequency + 0.5) / (docFrequency + 0.5) + 1);
    float normTermFreq = termFrequency * (k1 + 1) / (termFrequency + k1 * (1 - b + b * (docLength / avgDocLength)));
    return idf * normTermFreq;
}
} // anonymous namespace

std::unique_ptr<BM25Manager> BM25Manager::instance;
std::mutex BM25Manager::instanceMutex;

BM25Manager::BM25Manager() {
}

BM25Manager& BM25Manager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new BM25Manager());
    }
    return *instance;
}

void BM25Manager::addDocument(long vectorId, const std::string& doc, const std::vector<std::string>& tokens) {
    auto& db = DatabaseManager::getInstance().getDatabase();

    std::string tokensSerialized;
    for (const auto& token : tokens) {
        if (!tokensSerialized.empty()) {
            tokensSerialized += " ";
        }
        tokensSerialized += token + ":1";
    }

    spdlog::debug("Adding document: vectorId={}, doc={}, tokens={}", vectorId, doc, tokensSerialized);

    SQLite::Transaction transaction(db);
    SQLite::Statement insertQuery(db, "INSERT INTO BM25 (vectorId, doc, docLength, tokens) VALUES (?, ?, ?, ?)");
    insertQuery.bind(1, vectorId);
    insertQuery.bind(2, doc);
    insertQuery.bind(3, static_cast<int>(tokens.size()));
    insertQuery.bind(4, tokensSerialized);
    insertQuery.exec();
    transaction.commit();
}

std::vector<std::pair<long, double>> BM25Manager::searchWithVectorIds(
    const std::vector<long>& vectorIds,
    const std::vector<std::string>& queryTokens) {
    auto& db = DatabaseManager::getInstance().getDatabase();

    // Prepare IN query for vectorIds
    std::ostringstream idListStream;
    idListStream << "(";
    for (size_t i = 0; i < vectorIds.size(); ++i) {
        idListStream << vectorIds[i];
        if (i < vectorIds.size() - 1) {
            idListStream << ", ";
        }
    }
    idListStream << ")";
    std::string idList = idListStream.str();

    // Fetch tokens and document lengths for vectorIds
    std::string queryStr = "SELECT vectorId, tokens, docLength FROM BM25 WHERE vectorId IN " + idList;
    SQLite::Statement query(db, queryStr);

    double avgDocLength = 0.0;
    size_t totalDocs = 0;
    std::unordered_map<long, std::unordered_map<std::string, int>> vectorTokenFrequencies;
    std::unordered_map<long, double> vectorDocLengths;

    // Load data into memory
    while (query.executeStep()) {
        long vectorId = query.getColumn(0).getInt64();
        std::string serializedTokens = query.getColumn(1).getString();
        double docLength = query.getColumn(2).getDouble();

        avgDocLength += docLength;
        totalDocs++;

        vectorDocLengths[vectorId] = docLength;

        // Deserialize tokens with frequency
        std::unordered_map<std::string, int> tokenFrequency;
        std::istringstream tokenStream(serializedTokens);
        std::string tokenEntry;
        while (std::getline(tokenStream, tokenEntry, ' ')) {
            size_t colonPos = tokenEntry.find(':');
            if (colonPos != std::string::npos) {
                std::string token = tokenEntry.substr(0, colonPos);
                int frequency = std::stoi(tokenEntry.substr(colonPos + 1));
                tokenFrequency[token] = frequency;
            }
        }

        vectorTokenFrequencies[vectorId] = tokenFrequency;
    }

    if (totalDocs > 0) {
        avgDocLength /= totalDocs;
    }

    // Calculate document frequencies for query tokens
    std::unordered_map<std::string, int> queryTokenDocFrequencies;
    for (const auto& queryToken : queryTokens) {
        int docFrequency = 0;
        for (const auto& [vectorId, tokenFrequency] : vectorTokenFrequencies) {
            if (tokenFrequency.find(queryToken) != tokenFrequency.end()) {
                docFrequency++;
            }
        }
        queryTokenDocFrequencies[queryToken] = docFrequency;
    }

    // Calculate BM25 scores
    std::unordered_map<long, double> bm25Scores;
    for (const auto& [vectorId, tokenFrequency] : vectorTokenFrequencies) {
        double score = 0.0;
        double docLength = vectorDocLengths[vectorId];

        for (const auto& queryToken : queryTokens) {
            double termFrequency = 0.0;

            // Check if the token exists in the tokenFrequency map
            if (tokenFrequency.find(queryToken) != tokenFrequency.end()) {
                termFrequency = tokenFrequency.at(queryToken); // Use at() instead of []
            }

            if (termFrequency > 0) {
                double docFrequency = queryTokenDocFrequencies[queryToken];
                score += calculateBM25(termFrequency, docFrequency, docLength, avgDocLength, totalDocs);
            }
        }

        bm25Scores[vectorId] = score;
    }

    // Convert scores to a sorted vector
    std::vector<std::pair<long, double>> results(bm25Scores.begin(), bm25Scores.end());
    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    return results;
}

std::string BM25Manager::getDocByVectorId(int vectorId) {
    auto& db = DatabaseManager::getInstance().getDatabase();
    SQLite::Statement query(db, "SELECT doc FROM BM25 WHERE vectorId = ?");
    query.bind(1, vectorId);

    if (query.executeStep()) {
        return query.getColumn(0).getString();
    } else {
        return "";
    }
}


} // namespace atinyvectors
