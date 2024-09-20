#ifndef __ATINYVECTORS_SPACE_HPP__
#define __ATINYVECTORS_SPACE_HPP__

#include <string>
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <mutex>
#include <vector>

namespace atinyvectors
{

class Space {
public:
    int id;
    std::string name;
    std::string description;
    long created_time_utc;
    long updated_time_utc;

    Space() : id(0), created_time_utc(0), updated_time_utc(0) {}

    Space(int id, const std::string& name, const std::string& description, long created_time_utc, long updated_time_utc)
        : id(id), name(name), description(description), created_time_utc(created_time_utc), updated_time_utc(updated_time_utc) {}
};

class SpaceManager {
private:
    static std::unique_ptr<SpaceManager> instance;
    static std::mutex instanceMutex;

    SpaceManager();
public:
    SpaceManager(const SpaceManager&) = delete;
    SpaceManager& operator=(const SpaceManager&) = delete;

    static SpaceManager& getInstance();
    
    void createTable();

    int addSpace(Space& space);
    std::vector<Space> getAllSpaces();
    Space getSpaceById(int id);
    Space getSpaceByName(const std::string& name);
    void updateSpace(Space& space);
    void deleteSpace(int id);
};

}

#endif