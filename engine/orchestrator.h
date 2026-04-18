#pragma once
#include <string>
#include <vector>
#include <utility>

struct ProjectFiles {
    std::string              projectId;
    std::vector<std::string> codeFilePaths;
};

struct ModelResponse {
    std::string textual;
    // Each pair is (fileName, compressedContent).
    std::vector<std::pair<std::string, std::string>> files;
};

struct ApiUsage {
    long input_tokens  = 0;
    long output_tokens = 0;
};

// A single file entry carrying its detected language, name, and content.
// Used by buildPrompt to group files under [LANGUAGE: X] section headers.
struct CompressedFile {
    std::string language;   // "C", "C++", "Go", etc.
    std::string fileName;
    std::string content;
};

class Orchestrator {
public:
    // Compresses all project files, calls the model, then decompresses
    // the response.  Returns the decompressed textual reply.
    static std::string run(const std::string&              userQuery,
                           const std::vector<ProjectFiles>& projects);

private:
    static std::string buildPrompt(
        const std::string& userQuery,
        const std::vector<CompressedFile>& files,
        const std::vector<std::pair<std::string, std::string>>& symbolTable);

    static std::string callModel(const std::string& prompt, ApiUsage& outUsage,
                                 std::string& outRawJson,
                                 std::string& outRequestJson);

    static ModelResponse parseModelResponse(const std::string& rawResponse);
};
