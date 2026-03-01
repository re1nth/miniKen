#include "orchestrator.h"

#include "sessionStore/sessionMapObject.h"
#include "parser/querySquasher.h"
#include "parser/responseBuilder.h"
#include "parser/cppProfile.h"
#include "decompressor/relaxTextual.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <unistd.h>

// ---------------------------------------------------------------------------
// profileForPath — returns nullptr for non-parseable files
// ---------------------------------------------------------------------------

static const LanguageProfile* profileForPath(const std::string& filePath) {
    static const LanguageProfile kCpp = makeCppProfile();

    auto ext = [&]() -> std::string {
        auto dot = filePath.rfind('.');
        if (dot == std::string::npos) return {};
        return filePath.substr(dot);
    }();

    if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" ||
        ext == ".c"   || ext == ".h"  || ext == ".hpp" || ext == ".hxx")
        return &kCpp;

    return nullptr;
}

// ---------------------------------------------------------------------------
// buildPrompt
// ---------------------------------------------------------------------------

std::string Orchestrator::buildPrompt(
    const std::string& userQuery,
    const std::vector<std::pair<std::string, std::string>>& compressedFiles) {

    // Load model instructions from static resource.
    std::string instructions;
    {
        std::ifstream f("engine/static/prompt_format", std::ios::binary);
        if (f) {
            std::ostringstream ss;
            ss << f.rdbuf();
            instructions = ss.str();
        }
    }

    std::string prompt;
    prompt.reserve(4096);

    if (!instructions.empty()) {
        prompt += instructions;
        prompt += "\n\n";
    }

    prompt += "USER QUERY:\n";
    prompt += userQuery;
    prompt += "\n\n";

    for (const auto& [name, content] : compressedFiles) {
        prompt += "[FILE: ";
        prompt += name;
        prompt += "]\n";
        prompt += content;
        prompt += "\n";
    }

    return prompt;
}

// ---------------------------------------------------------------------------
// callModel — calls the Anthropic API when ANTHROPIC_API_KEY is set,
//             otherwise falls back to the stub.
// ---------------------------------------------------------------------------

// Escape a raw string so it is safe to embed inside a JSON string value.
static std::string jsonEscape(const std::string& raw) {
    std::string out;
    out.reserve(raw.size() + 64);
    for (unsigned char c : raw) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

std::string Orchestrator::callModel(const std::string& prompt) {
    const char* apiKey = std::getenv("ANTHROPIC_API_KEY");
    if (!apiKey || std::string(apiKey).empty()) {
        std::cerr << "[miniKen] ANTHROPIC_API_KEY not set — using stub response.\n";
        return "(stub response — set ANTHROPIC_API_KEY to enable real calls)\n";
    }

    const char* model = std::getenv("MINIKEN_MODEL");
    std::string modelId = model ? model : "claude-sonnet-4-6";

    // Write the JSON body to a temp file to avoid shell-quoting issues.
    char tmpPath[] = "/tmp/miniken_req_XXXXXX";
    int fd = mkstemp(tmpPath);
    if (fd < 0) {
        std::cerr << "[miniKen] mkstemp failed\n";
        return {};
    }

    std::string body =
        "{\"model\":\"" + modelId + "\","
        "\"max_tokens\":8192,"
        "\"messages\":[{\"role\":\"user\",\"content\":\""
        + jsonEscape(prompt) +
        "\"}]}";

    ::write(fd, body.c_str(), body.size());
    ::close(fd);

    // Build the curl | jq command.
    std::string cmd =
        "curl -s https://api.anthropic.com/v1/messages"
        " -H \"x-api-key: " + std::string(apiKey) + "\""
        " -H \"anthropic-version: 2023-06-01\""
        " -H \"content-type: application/json\""
        " -d @" + tmpPath +
        " | jq -r '.content[0].text' 2>/dev/null";

    std::string response;
    {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "[miniKen] popen failed\n";
            std::remove(tmpPath);
            return {};
        }
        char buf[4096];
        while (std::fgets(buf, sizeof(buf), pipe))
            response += buf;
        pclose(pipe);
    }

    std::remove(tmpPath);

    // jq outputs "null" when the field is missing (e.g. API error).
    if (response.empty() || response.substr(0, 4) == "null") {
        std::cerr << "[miniKen] Unexpected API response — check your API key "
                     "and model name.\n";
        return {};
    }

    // Trim trailing newline jq adds.
    if (!response.empty() && response.back() == '\n')
        response.pop_back();

    return response;
}

// ---------------------------------------------------------------------------
// parseModelResponse
// ---------------------------------------------------------------------------

ModelResponse Orchestrator::parseModelResponse(const std::string& rawResponse) {
    ModelResponse result;

    // Split on [FILE: <name>] delimiters.
    static const std::regex kFileHeader(R"(\[FILE:\s*([^\]]+)\])");

    std::string textualPart;
    std::sregex_iterator it(rawResponse.begin(), rawResponse.end(), kFileHeader);
    std::sregex_iterator end;

    // Everything before the first [FILE:...] is the textual preamble.
    if (it == end) {
        // No file sections — entire response is textual.
        textualPart = rawResponse;
    } else {
        textualPart = rawResponse.substr(0, static_cast<std::size_t>(it->position()));

        std::vector<std::pair<std::size_t, std::string>> headers;
        for (; it != end; ++it) {
            headers.push_back({ static_cast<std::size_t>(it->position() +
                                                          it->length()),
                                 (*it)[1].str() });
        }

        for (std::size_t i = 0; i < headers.size(); ++i) {
            std::size_t contentStart = headers[i].first;
            std::size_t nextHeader   = rawResponse.find("[FILE:", contentStart);
            if (nextHeader == std::string::npos) nextHeader = rawResponse.size();

            std::string content = rawResponse.substr(contentStart,
                                                     nextHeader - contentStart);
            result.files.emplace_back(headers[i].second, content);
        }
    }

    // Strip optional [TEXTUAL] marker if the stub used it.
    static const std::regex kTextualTag(R"(\[TEXTUAL\]\s*)");
    textualPart = std::regex_replace(textualPart, kTextualTag, "");

    result.textual = textualPart;
    return result;
}

// ---------------------------------------------------------------------------
// run
// ---------------------------------------------------------------------------

std::string Orchestrator::run(const std::string&              userQuery,
                              const std::vector<ProjectFiles>& projects) {
    SessionMapObject session;

    // Step 1: compress all code files.
    std::vector<std::pair<std::string, std::string>> compressedFiles;
    // Track which projectId each compressed file belongs to.
    std::vector<std::pair<std::string, std::string>> fileToProject; // fileName → projectId

    for (const auto& proj : projects) {
        for (const auto& filePath : proj.codeFilePaths) {
            std::string fileName = filePath;
            if (auto pos = filePath.rfind('/'); pos != std::string::npos)
                fileName = filePath.substr(pos + 1);

            const LanguageProfile* profile = profileForPath(filePath);
            std::string compressed;
            if (profile) {
                compressed = QuerySquasher::apply(session, proj.projectId, filePath, *profile);
            } else {
                // Non-parseable file: pass content through unmodified.
                std::ifstream f(filePath, std::ios::binary);
                if (f) {
                    std::ostringstream ss;
                    ss << f.rdbuf();
                    compressed = ss.str();
                }
            }

            compressedFiles.emplace_back(fileName, compressed);
            fileToProject.emplace_back(fileName, proj.projectId);
        }
    }

    // Step 2: build prompt and call model (stub).
    std::string prompt      = buildPrompt(userQuery, compressedFiles);
    std::string rawResponse = callModel(prompt);

    // Step 3: parse model response.
    ModelResponse modelResp = parseModelResponse(rawResponse);

    // Step 4: decompress modified files.
    // We need the projectId for each returned file.  Use first project as default
    // when there's only one; otherwise match by name.
    auto getProjectId = [&](const std::string& name) -> std::string {
        for (const auto& [fn, pid] : fileToProject) {
            if (fn == name) return pid;
        }
        return projects.empty() ? "" : projects[0].projectId;
    };

    for (auto& [name, content] : modelResp.files) {
        std::string pid = getProjectId(name);
        const LanguageProfile* profile = profileForPath(name);
        if (profile) {
            content = ResponseBuilder::apply(session, pid, content, *profile);
        }
    }

    // Step 5: decompress textual response.
    // Use first project id for textual (markers refer to the shared symbol space).
    std::string pid = projects.empty() ? "" : projects[0].projectId;
    return RelaxTextual::apply(session, pid, modelResp.textual);
}
