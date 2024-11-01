#ifndef __ATINYVECTORS_FILTER_MANAGER_HPP__
#define __ATINYVECTORS_FILTER_MANAGER_HPP__

#include <memory>
#include <mutex>
#include <string>

namespace atinyvectors {
namespace filter {

class FilterManager {
private:
    static std::unique_ptr<FilterManager> instance;
    static std::mutex instanceMutex;

    FilterManager();

    std::string convertFilterToSQL(const std::string& filter);

public:
    FilterManager(const FilterManager&) = delete;
    FilterManager& operator=(const FilterManager&) = delete;

    static FilterManager& getInstance();

    std::string toSQL(const std::string& filter);
};

};
};

#endif // FILTER_MANAGER_HPP
