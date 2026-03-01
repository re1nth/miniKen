#include "commentOut.h"

void CommentOut::apply(SessionMapObject& session,
                       const std::string& /*projectId*/,
                       const std::string& fileName,
                       const std::string& commentBlock,
                       const std::string& attachedId) {
    session.storeCommentOut(fileName, attachedId, commentBlock);
    // The comment is intentionally dropped from the compressed output.
}
