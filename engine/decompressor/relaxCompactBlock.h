#pragma once
#include <string>
#include "../sessionStore/sessionMapObject.h"

// Maps a compact symbol back to its original identifier.
// Returns "" if the component was newly added (keep compact id as-is).
class RelaxCompactBlock {
public:
    static std::string apply(SessionMapObject& session,
                             const std::string& projectId,
                             const std::string& fileName,
                             const std::string& componentType,
                             const std::string& componentId);
};
