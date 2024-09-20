#include "Config.hpp"

namespace {

// This function will be automatically called when the shared library is loaded
__attribute__((constructor)) void on_load() {
    atinyvectors::Config& config = atinyvectors::Config::getInstance();
    spdlog::info("Config singleton instance has been created upon .so loading.");
}

}

namespace atinyvectors {
    
std::unique_ptr<Config> Config::instance_ = nullptr;

}
