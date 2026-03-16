#include "relaxTextual.h"

#include <regex>
#include <string>

// ---------------------------------------------------------------------------
// Resolve a single compact id, handling compound forms like v1::v5 or v4.v10
// by splitting on '::' / '.' separators, looking up each part individually,
// and rejoining with the original separator.
// ---------------------------------------------------------------------------
static std::string resolveId(const SessionMapObject& session,
                             const std::string& projectId,
                             const std::string& compactId) {
    // Fast path: direct hit (simple id with no separator).
    std::string direct = session.getCompactBlockId(projectId, compactId);
    if (!direct.empty()) return direct;

    // Slow path: walk through the id, splitting on '::' and '.' in order.
    std::string result;
    std::string remaining = compactId;

    while (!remaining.empty()) {
        std::size_t colonPos = remaining.find("::");
        std::size_t dotPos   = remaining.find('.');

        // No more separators — last (or only) part.
        if (colonPos == std::string::npos && dotPos == std::string::npos) {
            std::string orig = session.getCompactBlockId(projectId, remaining);
            result += orig.empty() ? remaining : orig;
            break;
        }

        std::size_t sepPos;
        std::string sep;
        if (colonPos != std::string::npos &&
            (dotPos == std::string::npos || colonPos < dotPos)) {
            sepPos = colonPos;
            sep    = "::";
        } else {
            sepPos = dotPos;
            sep    = ".";
        }

        std::string part = remaining.substr(0, sepPos);
        std::string orig = session.getCompactBlockId(projectId, part);
        result   += orig.empty() ? part : orig;
        result   += sep;
        remaining = remaining.substr(sepPos + sep.size());
    }

    return result;
}

std::string RelaxTextual::apply(SessionMapObject& session,
                                const std::string& projectId,
                                const std::string& textualResponse) {
    // Match {{anything}} — capture group 1 is the compact id.
    static const std::regex kMarker(R"(\{\{([^}]+)\}\})");

    std::string result;
    result.reserve(textualResponse.size());

    auto begin = std::sregex_iterator(textualResponse.begin(),
                                      textualResponse.end(),
                                      kMarker);
    auto end   = std::sregex_iterator();

    std::size_t lastPos = 0;
    for (auto it = begin; it != end; ++it) {
        const std::smatch& m = *it;

        // Copy text before this match unchanged.
        result.append(textualResponse, lastPos,
                      static_cast<std::size_t>(m.position()) - lastPos);

        result += resolveId(session, projectId, m[1].str());

        lastPos = static_cast<std::size_t>(m.position()) +
                  static_cast<std::size_t>(m.length());
    }

    // Append any trailing text after the last match.
    result.append(textualResponse, lastPos, std::string::npos);
    return result;
}
