#include "compactBlock.h"

std::string CompactBlock::apply(SessionMapObject& session,
                                const std::string& projectId,
                                char componentType,
                                const std::string& originalName) {
    return session.allocateCompactId(projectId, componentType, originalName);
}
