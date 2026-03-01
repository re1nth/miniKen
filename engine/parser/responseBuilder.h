#pragma once
#include <string>
#include "../sessionStore/sessionMapObject.h"
#include "languageProfile.h"

// Mirrors QuerySquasher: takes a compressed source file (as returned by the
// LLM), decompresses identifiers and re-attaches stored comment blocks.
class ResponseBuilder {
public:
    static std::string apply(SessionMapObject& session,
                             const std::string& projectId,
                             const std::string& compressedSource,
                             const LanguageProfile& profile);
};
