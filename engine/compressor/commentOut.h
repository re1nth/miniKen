#pragma once
#include <string>
#include "../sessionStore/sessionMapObject.h"

// Strips a comment block from the compressed output and stores it in the
// session so it can be restored by the decompressor later.
class CommentOut {
public:
    // Stores commentBlock in the session keyed by (fileName, attachedId).
    // Returns nothing — the comment is dropped from the compressed stream.
    static void apply(SessionMapObject& session,
                      const std::string& projectId,
                      const std::string& fileName,
                      const std::string& commentBlock,
                      const std::string& attachedId);
};
