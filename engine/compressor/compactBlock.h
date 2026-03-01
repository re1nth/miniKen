#pragma once
#include <string>
#include "../sessionStore/sessionMapObject.h"

// Maps a long identifier to a short compact symbol (e.g. "f1", "C2") and
// stores the bidirectional mapping in the session.
class CompactBlock {
public:
    // Returns the compact symbol for originalName, allocating one if needed.
    // componentType: 'f' = function/method, 'C' = class/struct,
    //                'v' = variable,          'p' = package/include
    static std::string apply(SessionMapObject& session,
                             const std::string& projectId,
                             char componentType,
                             const std::string& originalName);
};
