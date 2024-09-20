#ifndef __ATINYVECTORS_VECTOR_DTO_HPP__
#define __ATINYVECTORS_VECTOR_DTO_HPP__

#include <string>
#include "nlohmann/json.hpp"
#include "Version.hpp"
#include "Vector.hpp"
#include "VectorMetadata.hpp"

using namespace nlohmann;

namespace atinyvectors {
namespace dto {

class VectorDTOManager {
public:
    void upsert(const std::string& spaceName, int versionUniqueId, const std::string& jsonStr); 
    json getVectorsByVersionId(int versionId);

private:
    void processSimpleVectors(const json& vectorsJson, int versionId, int defaultIndexId);
    void processDefaultDenseData(const json& parsedJson, int versionId, int defaultIndexId);
    void processVectors(const json& parsedJson, int versionId, int defaultIndexId);
};

} // namespace dto
} // namespace atinyvectors

#endif // __VECTOR_DTO_HPP__
