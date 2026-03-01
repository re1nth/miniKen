#pragma once
#include <string>
#include "../sessionStore/sessionMapObject.h"
#include "languageProfile.h"

// Two-pass source compressor backed by tree-sitter.
//
// Pass 1 — walks definition nodes to build the compact-symbol map.
// Pass 2 — re-emits the source replacing every mapped identifier with its
//           compact symbol and dropping comment nodes entirely.
class QuerySquasher {
public:
    // Returns the compressed source string for filePath and populates session.
    static std::string apply(SessionMapObject& session,
                             const std::string& projectId,
                             const std::string& filePath,
                             const LanguageProfile& profile);
};
