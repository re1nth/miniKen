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

class Orchestrator {
public:
    // Compresses all project files, stubs a model call, then decompresses
    // the response.  Returns the decompressed textual reply.
    static std::string run(const std::string&              userQuery,
                           const std::vector<ProjectFiles>& projects);

private:
    static std::string buildPrompt(
        const std::string& userQuery,
        const std::vector<std::pair<std::string, std::string>>& compressedFiles);

    static std::string callModel(const std::string& prompt, ApiUsage& outUsage);

    static ModelResponse parseModelResponse(const std::string& rawResponse);
};
