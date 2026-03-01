#include "responseBuilder.h"
#include "languageProfile.h"

#include "../decompressor/relaxCommentOut.h"
#include "../decompressor/relaxCompactBlock.h"

#include <tree_sitter/api.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Name-finding helpers
// ---------------------------------------------------------------------------

// Walk a node's children (and optionally declarator wrappers) looking for
// the first leaf identifier. Returns a null node when nothing is found.
static TSNode findPrimaryIdentifier(TSNode node, const LanguageProfile& profile) {
    uint32_t cc = ts_node_child_count(node);

    // Direct identifier children first.
    for (uint32_t i = 0; i < cc; ++i) {
        TSNode child = ts_node_child(node, i);
        if (profile.identifierNodeTypes.count(ts_node_type(child)))
            return child;
    }

    // Descend into declarator wrappers (function_declarator, pointer_declarator…).
    for (uint32_t i = 0; i < cc; ++i) {
        TSNode child = ts_node_child(node, i);
        if (profile.declaratorWrappers.count(ts_node_type(child))) {
            TSNode nested = findPrimaryIdentifier(child, profile);
            if (!ts_node_is_null(nested)) return nested;
        }
    }

    return TSNode{};  // null node
}

// ---------------------------------------------------------------------------
// Event model for reconstruction
// ---------------------------------------------------------------------------

struct Event {
    uint32_t    start;
    uint32_t    end;
    std::string insertBefore;  // comment block to prepend (for definitions)
    std::string replacement;   // non-empty → replace bytes [start,end) with this
};

// ---------------------------------------------------------------------------
// Walk tree and collect events
// ---------------------------------------------------------------------------

static void collectEvents(TSNode node, const std::string& src,
                          SessionMapObject& session,
                          const std::string& projectId,
                          const std::string& fileName,
                          std::vector<Event>& events,
                          const LanguageProfile& profile) {
    const char* type = ts_node_type(node);

    if (profile.definitionNodeTypes.count(type)) {
        TSNode identNode = findPrimaryIdentifier(node, profile);
        if (!ts_node_is_null(identNode)) {
            uint32_t s         = ts_node_start_byte(identNode);
            uint32_t e         = ts_node_end_byte(identNode);
            std::string compactId = src.substr(s, e - s);

            // Restore comment (may be "").
            std::string comment = RelaxCommentOut::apply(
                session, projectId, fileName, type, compactId);

            // Restore original name (may be "" → keep compact id).
            std::string original = RelaxCompactBlock::apply(
                session, projectId, fileName, type, compactId);

            Event ev{};
            ev.start        = ts_node_start_byte(node);
            ev.end          = ts_node_start_byte(node);  // zero-width insert
            ev.insertBefore = comment.empty() ? "" : comment + "\n";
            events.push_back(ev);

            if (!original.empty()) {
                Event idEv{};
                idEv.start       = s;
                idEv.end         = e;
                idEv.replacement = original;
                events.push_back(idEv);
            }
        }
    } else if (profile.identifierNodeTypes.count(type) && ts_node_child_count(node) == 0) {
        uint32_t s         = ts_node_start_byte(node);
        uint32_t e         = ts_node_end_byte(node);
        std::string compactId = src.substr(s, e - s);
        std::string original  = RelaxCompactBlock::apply(
            session, projectId, fileName, type, compactId);
        if (!original.empty()) {
            events.push_back({s, e, {}, original});
        }
        return;  // don't recurse into leaf
    }

    uint32_t cc = ts_node_child_count(node);
    for (uint32_t i = 0; i < cc; ++i) {
        collectEvents(ts_node_child(node, i), src, session, projectId,
                      fileName, events, profile);
    }
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

std::string ResponseBuilder::apply(SessionMapObject& session,
                                   const std::string& projectId,
                                   const std::string& compressedSource,
                                   const LanguageProfile& profile) {
    // We use "compressed_response" as a synthetic file name for comment lookup.
    const std::string fileName = "compressed_response";

    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, profile.grammar);

    TSTree* tree = ts_parser_parse_string(parser, nullptr,
                                          compressedSource.c_str(),
                                          static_cast<uint32_t>(compressedSource.size()));
    TSNode root  = ts_tree_root_node(tree);

    std::vector<Event> events;
    collectEvents(root, compressedSource, session, projectId, fileName, events, profile);

    // Sort events by start byte; stable sort preserves comment-before-definition order.
    std::stable_sort(events.begin(), events.end(),
                     [](const Event& a, const Event& b){ return a.start < b.start; });

    // Reconstruct output.
    std::string out;
    out.reserve(compressedSource.size() * 2);
    uint32_t pos = 0;

    for (const auto& ev : events) {
        if (ev.start < pos) continue;  // skip overlapping events
        out.append(compressedSource, pos, ev.start - pos);
        if (!ev.insertBefore.empty()) out += ev.insertBefore;
        if (!ev.replacement.empty()) {
            out += ev.replacement;
            pos = ev.end;
        } else {
            pos = ev.start;  // zero-width insert
        }
    }
    out.append(compressedSource, pos, compressedSource.size() - pos);

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    return out;
}
