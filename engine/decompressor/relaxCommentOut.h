#pragma once
#include <string>
#include "../sessionStore/sessionMapObject.h"

// Retrieves the comment block that was stripped during compression.
// Returns "" if the component was newly added (no stored comment).
class RelaxCommentOut {
public:
    static std::string apply(SessionMapObject& session,
                             const std::string& projectId,
                             const std::string& fileName,
                             const std::string& componentType,
                             const std::string& componentId);
};
