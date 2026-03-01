// test_compressor.cpp
// Unit tests for CommentOut and CompactBlock compressor modules.
#include "../testRunner.h"
#include "sessionStore/sessionMapObject.h"
#include "compressor/commentOut.h"
#include "compressor/compactBlock.h"

// ─── CommentOut::apply ────────────────────────────────────────────────────────

TEST(commentOut_apply_stores_in_session) {
    SessionMapObject s;
    CommentOut::apply(s, "projA", "utils.cpp",
                      "// Validates the order", "validateOrder");
    EXPECT_EQ(s.getCommentOut("utils.cpp", "validateOrder"),
              "// Validates the order");
}

TEST(commentOut_apply_uses_filename_not_project_as_key) {
    // CommentOut keys by fileName, not projectId.
    SessionMapObject s;
    CommentOut::apply(s, "proj1", "foo.cpp", "// docs", "myFunc");
    // Same fileName from a different project call still retrieves it.
    EXPECT_EQ(s.getCommentOut("foo.cpp", "myFunc"), "// docs");
    // A different fileName returns nothing.
    EXPECT_EQ(s.getCommentOut("bar.cpp", "myFunc"), "");
}

TEST(commentOut_apply_attachedId_is_the_lookup_key) {
    SessionMapObject s;
    CommentOut::apply(s, "p", "a.cpp", "// comment for f1", "f1");
    CommentOut::apply(s, "p", "a.cpp", "// comment for f2", "f2");
    EXPECT_EQ(s.getCommentOut("a.cpp", "f1"), "// comment for f1");
    EXPECT_EQ(s.getCommentOut("a.cpp", "f2"), "// comment for f2");
}

TEST(commentOut_apply_overwrites_on_same_key) {
    SessionMapObject s;
    CommentOut::apply(s, "p", "a.cpp", "// first", "C1");
    CommentOut::apply(s, "p", "a.cpp", "// second", "C1");
    EXPECT_EQ(s.getCommentOut("a.cpp", "C1"), "// second");
}

TEST(commentOut_apply_multiline_comment_preserved) {
    SessionMapObject s;
    std::string block = "/**\n * Processes the payment.\n * @throws PaymentException\n */";
    CommentOut::apply(s, "p", "payment.cpp", block, "f1");
    EXPECT_EQ(s.getCommentOut("payment.cpp", "f1"), block);
}

// ─── CompactBlock::apply ──────────────────────────────────────────────────────

TEST(compactBlock_apply_returns_compact_symbol) {
    SessionMapObject s;
    std::string id = CompactBlock::apply(s, "proj", 'f', "processCustomerOrder");
    EXPECT_FALSE(id.empty());
}

TEST(compactBlock_apply_function_gets_f_prefix) {
    SessionMapObject s;
    std::string id = CompactBlock::apply(s, "proj", 'f', "someFunction");
    EXPECT_EQ(id.substr(0, 1), "f");
}

TEST(compactBlock_apply_class_gets_C_prefix) {
    SessionMapObject s;
    std::string id = CompactBlock::apply(s, "proj", 'C', "SomeClass");
    EXPECT_EQ(id.substr(0, 1), "C");
}

TEST(compactBlock_apply_first_function_is_f1) {
    SessionMapObject s;
    EXPECT_EQ(CompactBlock::apply(s, "proj", 'f', "firstFunc"), "f1");
}

TEST(compactBlock_apply_second_function_is_f2) {
    SessionMapObject s;
    CompactBlock::apply(s, "proj", 'f', "firstFunc");
    EXPECT_EQ(CompactBlock::apply(s, "proj", 'f', "secondFunc"), "f2");
}

TEST(compactBlock_apply_deduplication_same_name_same_symbol) {
    SessionMapObject s;
    std::string id1 = CompactBlock::apply(s, "proj", 'f', "processOrder");
    std::string id2 = CompactBlock::apply(s, "proj", 'f', "processOrder");
    EXPECT_EQ(id1, id2);
}

TEST(compactBlock_apply_different_names_get_different_symbols) {
    SessionMapObject s;
    std::string a = CompactBlock::apply(s, "proj", 'f', "funcA");
    std::string b = CompactBlock::apply(s, "proj", 'f', "funcB");
    EXPECT_NE(a, b);
}

TEST(compactBlock_apply_stores_in_session_for_decompressor) {
    // After apply, the session's forward map must resolve compact → original.
    SessionMapObject s;
    std::string compact = CompactBlock::apply(s, "proj", 'f', "calculateDiscount");
    EXPECT_EQ(s.getCompactBlockId("proj", compact), "calculateDiscount");
}

TEST(compactBlock_apply_project_isolation) {
    SessionMapObject s;
    // Same name in different projects → each gets its own f1.
    std::string idA = CompactBlock::apply(s, "projA", 'f', "sharedName");
    std::string idB = CompactBlock::apply(s, "projB", 'f', "sharedName");
    // Both are "f1" (independent counters), but they map to "sharedName" in their own project.
    EXPECT_EQ(s.getCompactBlockId("projA", idA), "sharedName");
    EXPECT_EQ(s.getCompactBlockId("projB", idB), "sharedName");
}

// ─── CommentOut + CompactBlock interaction ────────────────────────────────────

TEST(compressor_comment_keyed_by_compact_id) {
    // The typical usage pattern: compact the function name first,
    // then store the comment using the compact id as the attachedId.
    SessionMapObject s;
    std::string compact = CompactBlock::apply(s, "proj", 'f',
                                              "sendConfirmationEmail");
    CommentOut::apply(s, "proj", "mailer.cpp",
                      "// Sends a confirmation email to the customer", compact);

    // The comment should be retrievable via the compact id.
    EXPECT_EQ(s.getCommentOut("mailer.cpp", compact),
              "// Sends a confirmation email to the customer");
    // And the compact id resolves to the original name.
    EXPECT_EQ(s.getCompactBlockId("proj", compact), "sendConfirmationEmail");
}

TEST(compressor_multiple_functions_with_comments) {
    SessionMapObject s;
    struct { const char* fn; const char* comment; } items[] = {
        { "validateOrder",      "// Validates all order items"     },
        { "applyDiscount",      "// Applies the customer discount" },
        { "sendInvoice",        "// Sends invoice via email"       },
    };
    for (auto& item : items) {
        std::string id = CompactBlock::apply(s, "shop", 'f', item.fn);
        CommentOut::apply(s, "shop", "order.cpp", item.comment, id);
    }
    // Verify each comment is retrievable.
    for (auto& item : items) {
        std::string id = s.allocateCompactId("shop", 'f', item.fn);
        EXPECT_EQ(s.getCommentOut("order.cpp", id), item.comment);
    }
}
