#include "relaxCompactBlock.h"

std::string RelaxCompactBlock::apply(SessionMapObject& session,
                                     const std::string& projectId,
                                     const std::string& /*fileName*/,
                                     const std::string& /*componentType*/,
                                     const std::string& componentId) {
    return session.getCompactBlockId(projectId, componentId);
}
