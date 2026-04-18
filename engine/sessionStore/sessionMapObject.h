#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class SessionMapObject {
public:
    SessionMapObject();

    const std::string& sessionId() const { return sessionId_; }

    // Write side
    void storeCommentOut(const std::string& fileName,
                         const std::string& attachedId,
                         const std::string& commentText);

    void storeCompactBlock(const std::string& projectId,
                           const std::string& compactId,
                           const std::string& originalName);

    // Read side
    std::string getCommentOut(const std::string& fileName,
                              const std::string& attachedId) const;

    std::string getCompactBlockId(const std::string& projectId,
                                  const std::string& compactId) const;

    // Allocate or retrieve a compact id for the given originalName.
    // componentType: 'f' → function, 'C' → class/struct, 'v' → variable, 'p' → package/include
    std::string allocateCompactId(const std::string& projectId,
                                  char componentType,
                                  const std::string& originalName);

    // Return all (compactId, originalName) pairs for a project, sorted by
    // prefix group (f, C, v, p) then by numeric suffix.
    std::vector<std::pair<std::string, std::string>> getCompactMappings(
        const std::string& projectId) const;

    // File → language mapping (e.g. "wal.c" → "C", "manager.cpp" → "C++").
    // Stored during compression so the decompressor can recover the right grammar.
    void storeFileLanguage(const std::string& fileName, const std::string& language);
    std::string getFileLanguage(const std::string& fileName) const;

private:
    std::string sessionId_;

    // "fileName\0attachedId" → commentText
    std::unordered_map<std::string, std::string> commentMap_;

    // "projectId\0compactId" → originalName
    std::unordered_map<std::string, std::string> compactFwdMap_;

    // "projectId\0originalName" → compactId
    std::unordered_map<std::string, std::string> compactRevMap_;

    // "projectId\0prefix" → next integer counter
    std::unordered_map<std::string, int> counters_;

    // fileName → language name ("C", "C++", "Go", …)
    std::unordered_map<std::string, std::string> fileLangMap_;

    static std::string makeKey(const std::string& a, const std::string& b);
};
