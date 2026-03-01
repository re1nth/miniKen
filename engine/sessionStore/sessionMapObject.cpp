#include "sessionMapObject.h"

#include <random>
#include <sstream>
#include <iomanip>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string generateSessionId() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << dist(gen)
        << std::setw(16) << dist(gen);
    return oss.str();
}

// Separator byte that is unlikely to appear in normal identifiers or filenames.
static constexpr char kSep = '\0';

std::string SessionMapObject::makeKey(const std::string& a, const std::string& b) {
    std::string key;
    key.reserve(a.size() + 1 + b.size());
    key = a;
    key += kSep;
    key += b;
    return key;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

SessionMapObject::SessionMapObject()
    : sessionId_(generateSessionId()) {}

// ---------------------------------------------------------------------------
// Write side
// ---------------------------------------------------------------------------

void SessionMapObject::storeCommentOut(const std::string& fileName,
                                       const std::string& attachedId,
                                       const std::string& commentText) {
    commentMap_[makeKey(fileName, attachedId)] = commentText;
}

void SessionMapObject::storeCompactBlock(const std::string& projectId,
                                         const std::string& compactId,
                                         const std::string& originalName) {
    compactFwdMap_[makeKey(projectId, compactId)]    = originalName;
    compactRevMap_[makeKey(projectId, originalName)] = compactId;
}

// ---------------------------------------------------------------------------
// Read side
// ---------------------------------------------------------------------------

std::string SessionMapObject::getCommentOut(const std::string& fileName,
                                             const std::string& attachedId) const {
    auto it = commentMap_.find(makeKey(fileName, attachedId));
    return it != commentMap_.end() ? it->second : std::string{};
}

std::string SessionMapObject::getCompactBlockId(const std::string& projectId,
                                                 const std::string& compactId) const {
    auto it = compactFwdMap_.find(makeKey(projectId, compactId));
    return it != compactFwdMap_.end() ? it->second : std::string{};
}

// ---------------------------------------------------------------------------
// Counter / allocation
// ---------------------------------------------------------------------------

std::string SessionMapObject::allocateCompactId(const std::string& projectId,
                                                 char componentType,
                                                 const std::string& originalName) {
    // Fast path: already mapped?
    auto revKey = makeKey(projectId, originalName);
    auto revIt  = compactRevMap_.find(revKey);
    if (revIt != compactRevMap_.end()) {
        return revIt->second;
    }

    // Allocate next counter for (projectId, prefix).
    std::string prefix(1, componentType);
    auto counterKey = makeKey(projectId, prefix);
    int  next       = ++counters_[counterKey];  // default-initialises to 0, then increments

    std::string compactId = prefix + std::to_string(next);

    storeCompactBlock(projectId, compactId, originalName);
    return compactId;
}
