#include "relaxTextual.h"

#include <regex>
#include <string>

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

        const std::string compactId = m[1].str();
        const std::string original  = session.getCompactBlockId(projectId, compactId);

        // Use original name if found; otherwise fall back to the compact id.
        result += original.empty() ? compactId : original;

        lastPos = static_cast<std::size_t>(m.position()) +
                  static_cast<std::size_t>(m.length());
    }

    // Append any trailing text after the last match.
    result.append(textualResponse, lastPos, std::string::npos);
    return result;
}
