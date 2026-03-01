#include "querySquasher.h"
#include "languageProfile.h"

#include "../compressor/commentOut.h"
#include "../compressor/compactBlock.h"

#include <tree_sitter/api.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// File helpers
// ---------------------------------------------------------------------------

static std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("QuerySquasher: cannot open " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Extract the primary name identifier from a definition node.
// Handles the case where the name is nested one level inside a declarator wrapper.
// Returns "" when no name can be determined.
static std::string extractName(TSNode node, const std::string& src,
                                const LanguageProfile& profile) {
    uint32_t childCount = ts_node_child_count(node);

    // First pass: direct identifier children.
    for (uint32_t i = 0; i < childCount; ++i) {
        TSNode child = ts_node_child(node, i);
        if (profile.identifierNodeTypes.count(ts_node_type(child))) {
            uint32_t start = ts_node_start_byte(child);
            uint32_t end   = ts_node_end_byte(child);
            return src.substr(start, end - start);
        }
    }

    // Second pass: descend into declarator wrappers so we can find the name
    // in e.g. function_definition → function_declarator → identifier.
    for (uint32_t i = 0; i < childCount; ++i) {
        TSNode child = ts_node_child(node, i);
        if (profile.declaratorWrappers.count(ts_node_type(child))) {
            std::string nested = extractName(child, src, profile);
            if (!nested.empty()) return nested;
        }
    }
    return {};
}

// ---------------------------------------------------------------------------
// Pass 1: walk tree and register all top-level definitions
// ---------------------------------------------------------------------------

static void pass1(TSNode node, const std::string& src,
                  SessionMapObject& session, const std::string& projectId,
                  const std::string& fileName,
                  std::string& pendingComment,
                  const LanguageProfile& profile) {
    const char* type = ts_node_type(node);

    if (profile.commentNodeTypes.count(type)) {
        uint32_t s = ts_node_start_byte(node);
        uint32_t e = ts_node_end_byte(node);
        pendingComment = src.substr(s, e - s);
        return;
    }

    if (profile.definitionNodeTypes.count(type)) {
        std::string name = extractName(node, src, profile);
        if (!name.empty()) {
            auto it = profile.componentTypeMap.find(type);
            char ct = (it != profile.componentTypeMap.end())
                       ? it->second
                       : profile.defaultComponentType;
            std::string compact = CompactBlock::apply(session, projectId, ct, name);

            if (!pendingComment.empty()) {
                CommentOut::apply(session, projectId, fileName,
                                  pendingComment, compact);
                pendingComment.clear();
            }
        }
    } else {
        pendingComment.clear();
    }

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) {
        pass1(ts_node_child(node, i), src, session, projectId, fileName,
              pendingComment, profile);
    }
}

// ---------------------------------------------------------------------------
// Pass 2: collect leaf identifier ranges and reconstruct compressed source
// ---------------------------------------------------------------------------

struct Replacement {
    uint32_t    start;
    uint32_t    end;
    std::string text;
};

static void collectReplacements(TSNode node, const std::string& src,
                                SessionMapObject& session,
                                const std::string& projectId,
                                std::vector<Replacement>& replacements,
                                std::vector<std::pair<uint32_t,uint32_t>>& skipRanges,
                                const LanguageProfile& profile) {
    const char* type = ts_node_type(node);

    // Skip comment nodes entirely — record their byte ranges.
    if (profile.commentNodeTypes.count(type)) {
        skipRanges.push_back({ ts_node_start_byte(node), ts_node_end_byte(node) });
        return;
    }

    if (profile.identifierNodeTypes.count(type) && ts_node_child_count(node) == 0) {
        uint32_t s    = ts_node_start_byte(node);
        uint32_t e    = ts_node_end_byte(node);
        std::string name = src.substr(s, e - s);

        // allocateCompactId fast-paths on pre-registered names (from pass 1),
        // returning their compact id regardless of the prefix argument.
        std::string compact = session.allocateCompactId(
            projectId, profile.defaultComponentType, name);
        if (!compact.empty()) {
            std::string roundtrip = session.getCompactBlockId(projectId, compact);
            if (roundtrip == name) {
                replacements.push_back({s, e, compact});
            }
        }
        return;
    }

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) {
        collectReplacements(ts_node_child(node, i), src, session, projectId,
                            replacements, skipRanges, profile);
    }
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

std::string QuerySquasher::apply(SessionMapObject& session,
                                 const std::string& projectId,
                                 const std::string& filePath,
                                 const LanguageProfile& profile) {
    std::string src = readFile(filePath);

    // Extract fileName from path for comment storage.
    std::string fileName = filePath;
    if (auto pos = filePath.rfind('/'); pos != std::string::npos)
        fileName = filePath.substr(pos + 1);

    // Set up tree-sitter parser.
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, profile.grammar);

    TSTree* tree = ts_parser_parse_string(parser, nullptr,
                                          src.c_str(),
                                          static_cast<uint32_t>(src.size()));
    TSNode root  = ts_tree_root_node(tree);

    // Pass 1: build symbol map.
    std::string pendingComment;
    pass1(root, src, session, projectId, fileName, pendingComment, profile);

    // Pass 2: collect replacements and skips.
    std::vector<Replacement> replacements;
    std::vector<std::pair<uint32_t,uint32_t>> skipRanges;
    collectReplacements(root, src, session, projectId, replacements, skipRanges, profile);

    // Reconstruct source: interleave unchanged bytes with replacements,
    // omitting comment ranges.
    std::sort(replacements.begin(), replacements.end(),
              [](const Replacement& a, const Replacement& b){ return a.start < b.start; });
    std::sort(skipRanges.begin(), skipRanges.end());

    // Merge into a unified event list.
    struct Event { uint32_t start, end; std::string text; bool skip; };
    std::vector<Event> events;
    for (auto& r : replacements) events.push_back({r.start, r.end, r.text, false});
    for (auto& s : skipRanges)   events.push_back({s.first, s.second, {}, true});
    std::sort(events.begin(), events.end(),
              [](const Event& a, const Event& b){ return a.start < b.start; });

    std::string out;
    out.reserve(src.size());
    uint32_t pos = 0;
    for (const auto& ev : events) {
        if (ev.start < pos) continue;  // overlapping — skip
        out.append(src, pos, ev.start - pos);
        if (!ev.skip) out += ev.text;
        pos = ev.end;
    }
    out.append(src, pos, src.size() - pos);

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    return out;
}
