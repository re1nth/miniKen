#pragma once
#include <string>
#include "../sessionStore/sessionMapObject.h"

// Scans textual LLM response for {{compactId}} markers and replaces each
// with the original identifier from the session.  If no mapping exists,
// strips the braces and keeps the compact id.
class RelaxTextual {
public:
    static std::string apply(SessionMapObject& session,
                             const std::string& projectId,
                             const std::string& textualResponse);
};
