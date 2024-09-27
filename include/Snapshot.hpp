#ifndef __ATINYVECTORS_SNAPSHOT_HPP__
#define __ATINYVECTORS_SNAPSHOT_HPP__

#include <string>
#include <vector>
#include <mutex>
#include <memory>

namespace atinyvectors {

class Snapshot {
public:
    int id;
    std::string requestJson; 
    std::string fileName;
    long createdTimeUtc;

    Snapshot() 
        : id(0), createdTimeUtc(0) {}

    Snapshot(int id, const std::string& requestJson, const std::string& fileName, long createdTimeUtc)
        : id(id), requestJson(requestJson), fileName(fileName), createdTimeUtc(createdTimeUtc) {}
};

class SnapshotManager {
private:
    static std::unique_ptr<SnapshotManager> instance;
    static std::mutex instanceMutex;

    SnapshotManager();

public:
    SnapshotManager(const SnapshotManager&) = delete;
    SnapshotManager& operator=(const SnapshotManager&) = delete;

    static SnapshotManager& getInstance();

    void createTable();
    
    int createSnapshot(const std::vector<std::pair<std::string, int>>& versionInfoList, const std::string& fileName, const std::string& metaDirectory);
    void restoreSnapshot(const std::string& zipFileName, const std::string& targetDirectory);
    
    std::vector<Snapshot> getAllSnapshots();
    Snapshot getSnapshotById(int id);
    void deleteSnapshot(int id);
    void cleanupStorage();
};

}

#endif
