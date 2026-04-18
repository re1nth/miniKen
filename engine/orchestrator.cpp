#include "orchestrator.h"

#include "sessionStore/sessionMapObject.h"
#include "parser/querySquasher.h"
#include "parser/responseBuilder.h"
#include "parser/languageRegistry.h"
#include "decompressor/relaxTextual.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <unistd.h>

// ---------------------------------------------------------------------------
// buildPrompt
// ---------------------------------------------------------------------------

std::string Orchestrator::buildPrompt(
    const std::string& userQuery,
    const std::vector<CompressedFile>& files,
    const std::vector<std::pair<std::string, std::string>>& symbolTable) {

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

    // Emit the symbol table so the model has the full compact → original mapping.
    if (!symbolTable.empty()) {
        prompt += "SYMBOL TABLE (compact \u2192 original):\n";
        for (const auto& [compact, original] : symbolTable) {
            prompt += "  " + compact + " = " + original + "\n";
        }
        prompt += "\n";
    }

    prompt += "USER QUERY:\n";
    prompt += userQuery;
    prompt += "\n\n";

    // Emit files grouped under [LANGUAGE: X] section headers.
    // Files are expected to arrive pre-sorted by language.
    std::string currentLang;
    for (const auto& file : files) {
        if (file.language != currentLang) {
            currentLang = file.language;
            if (!currentLang.empty()) {
                prompt += "[LANGUAGE: " + currentLang + "]\n";
            }
        }
        prompt += "[FILE: " + file.fileName + "]\n";
        prompt += file.content;
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

// ---------------------------------------------------------------------------
// countTokens — calls /v1/messages/count_tokens for an exact token count.
// Returns 0 on failure.
// ---------------------------------------------------------------------------

static long countTokens(const std::string& prompt,
                        const std::string& apiKey,
                        const std::string& modelId) {
    char reqPath[] = "/tmp/miniken_ctok_XXXXXX";
    int fd = mkstemp(reqPath);
    if (fd < 0) return 0;

    std::string body =
        "{\"model\":\"" + modelId + "\","
        "\"messages\":[{\"role\":\"user\",\"content\":\""
        + jsonEscape(prompt) + "\"}]}";

    ::write(fd, body.c_str(), body.size());
    ::close(fd);

    char respPath[] = "/tmp/miniken_ctok_resp_XXXXXX";
    fd = mkstemp(respPath);
    if (fd < 0) { std::remove(reqPath); return 0; }
    ::close(fd);

    std::string curlCmd =
        "curl -s https://api.anthropic.com/v1/messages/count_tokens"
        " -H \"x-api-key: " + apiKey + "\""
        " -H \"anthropic-version: 2023-06-01\""
        " -H \"content-type: application/json\""
        " -d @" + std::string(reqPath) +
        " > " + std::string(respPath);

    std::system(curlCmd.c_str());
    std::remove(reqPath);

    std::string cmd = std::string("jq -r '.input_tokens' ") + respPath + " 2>/dev/null";
    std::string out;
    FILE* p = popen(cmd.c_str(), "r");
    if (p) {
        char buf[64];
        while (std::fgets(buf, sizeof(buf), p)) out += buf;
        pclose(p);
    }
    std::remove(respPath);

    if (!out.empty() && out.back() == '\n') out.pop_back();
    if (out.empty() || out == "null") return 0;
    try { return std::stol(out); } catch (...) { return 0; }
}

std::string Orchestrator::callModel(const std::string& prompt, ApiUsage& outUsage,
                                    std::string& outRawJson,
                                    std::string& outRequestJson) {
    const char* apiKey = std::getenv("ANTHROPIC_API_KEY");
    if (!apiKey || std::string(apiKey).empty()) {
        std::cerr << "[miniKen] ANTHROPIC_API_KEY not set — using stub response.\n";
        return "(stub response — set ANTHROPIC_API_KEY to enable real calls)\n";
    }

    const char* model = std::getenv("MINIKEN_MODEL");
    std::string modelId = model ? model : "claude-sonnet-4-6";

    // Write the JSON body to a temp file to avoid shell-quoting issues.
    char reqPath[] = "/tmp/miniken_req_XXXXXX";
    int fd = mkstemp(reqPath);
    if (fd < 0) { std::cerr << "[miniKen] mkstemp failed\n"; return {}; }

    std::string body =
        "{\"model\":\"" + modelId + "\","
        "\"max_tokens\":8192,"
        "\"messages\":[{\"role\":\"user\",\"content\":\""
        + jsonEscape(prompt) + "\"}]}";

    outRequestJson = body;

    ::write(fd, body.c_str(), body.size());
    ::close(fd);

    // Temp file for the raw API response JSON.
    char respPath[] = "/tmp/miniken_resp_XXXXXX";
    fd = mkstemp(respPath);
    if (fd < 0) {
        std::cerr << "[miniKen] mkstemp failed\n";
        std::remove(reqPath);
        return {};
    }
    ::close(fd);

    // curl — save full JSON to respPath.
    std::string curlCmd =
        "curl -s https://api.anthropic.com/v1/messages"
        " -H \"x-api-key: " + std::string(apiKey) + "\""
        " -H \"anthropic-version: 2023-06-01\""
        " -H \"content-type: application/json\""
        " -d @" + std::string(reqPath) +
        " > " + std::string(respPath);

    std::system(curlCmd.c_str());
    std::remove(reqPath);

    // Helper: run a jq filter against respPath, return trimmed output.
    auto jqGet = [&](const char* filter) -> std::string {
        std::string cmd = std::string("jq -r '") + filter + "' "
                        + respPath + " 2>/dev/null";
        std::string out;
        FILE* p = popen(cmd.c_str(), "r");
        if (!p) return {};
        char buf[4096];
        while (std::fgets(buf, sizeof(buf), p)) out += buf;
        pclose(p);
        if (!out.empty() && out.back() == '\n') out.pop_back();
        return out;
    };

    // Capture full raw JSON before deleting the temp file.
    {
        std::ifstream f(respPath, std::ios::binary);
        if (f) { std::ostringstream ss; ss << f.rdbuf(); outRawJson = ss.str(); }
    }

    std::string response   = jqGet(".content[0].text");
    std::string inTokStr   = jqGet(".usage.input_tokens");
    std::string outTokStr  = jqGet(".usage.output_tokens");

    std::remove(respPath);

    if (response.empty() || response.substr(0, 4) == "null") {
        std::cerr << "[miniKen] Unexpected API response — check your API key "
                     "and model name.\n";
        return {};
    }

    try { outUsage.input_tokens  = std::stol(inTokStr);  } catch (...) {}
    try { outUsage.output_tokens = std::stol(outTokStr); } catch (...) {}

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
    LanguageRegistry::init();

    SessionMapObject session;
    const std::string projectId = projects.empty() ? "" : projects[0].projectId;

    // Step 1: detect language, compress, and store filename→language mapping.
    std::vector<CompressedFile> originalFiles;    // for uncompressed token estimate
    std::vector<CompressedFile> compressedFiles;

    for (const auto& proj : projects) {
        for (const auto& filePath : proj.codeFilePaths) {
            std::string fileName = filePath;
            if (auto pos = filePath.rfind('/'); pos != std::string::npos)
                fileName = filePath.substr(pos + 1);

            const LanguageProfile* profile =
                LanguageRegistry::instance().forPath(filePath);

            // Read original content (needed for both paths below).
            std::string original;
            {
                std::ifstream f(filePath, std::ios::binary);
                if (f) { std::ostringstream ss; ss << f.rdbuf(); original = ss.str(); }
            }

            if (!profile) {
                // Unknown extension — pass the file through uncompressed.
                session.storeFileLanguage(fileName, "raw");
                originalFiles.push_back({"raw", fileName, original});
                compressedFiles.push_back({"raw", fileName, original});
                continue;
            }

            const std::string& langStr = profile->name;

            // Store the filename → language mapping for decompression lookup.
            session.storeFileLanguage(fileName, langStr);

            originalFiles.push_back({langStr, fileName, original});

            std::string compressed = QuerySquasher::apply(
                session, proj.projectId, filePath, *profile);
            compressedFiles.push_back({langStr, fileName, compressed});
        }
    }

    // Sort both lists by language so [LANGUAGE: X] blocks are contiguous in the prompt.
    auto byLanguage = [](const CompressedFile& a, const CompressedFile& b) {
        return a.language < b.language;
    };
    std::stable_sort(originalFiles.begin(),   originalFiles.end(),   byLanguage);
    std::stable_sort(compressedFiles.begin(), compressedFiles.end(), byLanguage);

    // Step 2: collect symbol table and build both prompts.
    auto symbolTable = session.getCompactMappings(projectId);

    // Uncompressed prompt has no symbol table (it uses real names already).
    std::string uncompressedPrompt = buildPrompt(userQuery, originalFiles, {});
    std::string compressedPrompt   = buildPrompt(userQuery, compressedFiles, symbolTable);

    // Step 3: count uncompressed tokens (exact, via /v1/messages/count_tokens).
    const char* apiKey = std::getenv("ANTHROPIC_API_KEY");
    const char* model  = std::getenv("MINIKEN_MODEL");
    std::string modelId = model ? model : "claude-sonnet-4-6";

    long uncompressedTokens = 0;
    if (apiKey && std::string(apiKey).size() > 0)
        uncompressedTokens = countTokens(uncompressedPrompt, apiKey, modelId);

    // Step 4: call model, capture real token usage + raw JSON.
    ApiUsage usage;
    std::string rawApiJson;
    std::string rawRequestJson;
    std::string rawResponse = callModel(compressedPrompt, usage, rawApiJson, rawRequestJson);

    // Step 5: print token metrics to stdout (visible through MCP layer).
    {
        long actual = usage.input_tokens;
        std::cout << "\n--- miniKen Token Metrics ---\n";
        if (uncompressedTokens > 0) {
            std::cout << "Without compression: " << uncompressedTokens << " tokens\n";
        }
        if (actual > 0) {
            std::cout << "With compression:    " << actual << " tokens\n";
        }
        if (uncompressedTokens > 0 && actual > 0) {
            long saved = uncompressedTokens - actual;
            int  pct   = (int)(100.0 * saved / uncompressedTokens);
            std::cout << "Tokens saved:        " << saved << "  (" << pct << "%)\n";
        }
        if (usage.output_tokens > 0)
            std::cout << "Output tokens:       " << usage.output_tokens << "\n";
        std::cout << "-----------------------------\n\n";
    }

    // Emit the request JSON and raw API response so the web UI can display them.
    std::cout << "=== MINIKEN_COMPRESSED_PROMPT_BEGIN ===\n"
              << rawRequestJson
              << "\n=== MINIKEN_COMPRESSED_PROMPT_END ===\n";
    std::cout << "=== MINIKEN_RAW_API_RESPONSE_BEGIN ===\n"
              << rawApiJson
              << "\n=== MINIKEN_RAW_API_RESPONSE_END ===\n";

    // Step 5: parse model response.
    ModelResponse modelResp = parseModelResponse(rawResponse);

    // Step 6: decompress modified files.
    // Use the session's filename→language map (stored during compression) to
    // recover the correct grammar — this is especially important for .h files
    // whose language was determined by filesystem pairing, not extension alone.
    for (auto& [name, content] : modelResp.files) {
        std::string langStr = session.getFileLanguage(name);
        const LanguageProfile* profile =
            LanguageRegistry::instance().forName(langStr);
        if (profile) {
            content = ResponseBuilder::apply(session, projectId, content, *profile);
        }
        // "raw" files (unknown extension) have no profile — emit content as-is.
    }

    // Step 7: assemble the final response.
    // Apply RelaxTextual to the preamble, then re-attach each file section
    // (which ResponseBuilder already decompressed for code identifiers, but may
    // still contain {{marker}} references in surrounding text).
    std::string finalResponse = RelaxTextual::apply(session, projectId, modelResp.textual);

    for (const auto& [name, content] : modelResp.files) {
        finalResponse += "\n[FILE: " + name + "]\n";
        finalResponse += RelaxTextual::apply(session, projectId, content);
    }

    return finalResponse;
}
