#include "relaxCommentOut.h"

std::string RelaxCommentOut::apply(SessionMapObject& session,
                                   const std::string& /*projectId*/,
                                   const std::string& fileName,
                                   const std::string& /*componentType*/,
                                   const std::string& componentId) {
    // componentId is the compact symbol (e.g. "f1") that was used as the
    // attachedId when the comment was stored.
    return session.getCommentOut(fileName, componentId);
}
