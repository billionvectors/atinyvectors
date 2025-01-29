#ifndef __ATINYVECTORS_VECTOR_SERVICE_HPP__
#define __ATINYVECTORS_VECTOR_SERVICE_HPP__

#include <string>
#include "nlohmann/json.hpp"
#include "Version.hpp"
#include "Vector.hpp"
#include "VectorMetadata.hpp"

using namespace nlohmann;

namespace atinyvectors {
namespace service {

class VectorServiceManager {
public:
    void upsert(const std::string& spaceName, int versionUniqueId, const std::string& jsonStr); 
    json getVectorsByVersionId(const std::string& spaceName, int versionUniqueId, int start, int limit, const std::string& filter = "");

private:
    void processSimpleVectors(const json& vectorsJson, int versionId, int defaultIndexId);
    void processDefaultDenseData(const json& parsedJson, int versionId, int defaultIndexId);
    void processVectors(const json& parsedJson, int versionId, int defaultIndexId);
};

} // namespace service
} // namespace atinyvectors

#endif // __ATINYVECTORS_VECTOR_SERVICE_HPP__
