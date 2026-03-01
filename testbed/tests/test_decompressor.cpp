// test_decompressor.cpp
// Unit tests for RelaxCommentOut, RelaxCompactBlock, and RelaxTextual.
#include "../testRunner.h"
#include "sessionStore/sessionMapObject.h"
#include "decompressor/relaxCommentOut.h"
#include "decompressor/relaxCompactBlock.h"
#include "decompressor/relaxTextual.h"

// ─── RelaxCommentOut::apply ───────────────────────────────────────────────────

TEST(relaxCommentOut_returns_stored_comment) {
    SessionMapObject s;
    s.storeCommentOut("auth.cpp", "f1", "// Authenticates the user");
    std::string result = RelaxCommentOut::apply(s, "proj", "auth.cpp",
                                                "function_definition", "f1");
    EXPECT_EQ(result, "// Authenticates the user");
}

TEST(relaxCommentOut_returns_empty_for_unknown_component) {
    // A newly added component has no stored comment → return "".
    SessionMapObject s;
    std::string result = RelaxCommentOut::apply(s, "proj", "auth.cpp",
                                                "function_definition", "f99");
    EXPECT_EQ(result, "");
}

TEST(relaxCommentOut_componentType_is_not_part_of_lookup_key) {
    // The component type argument is informational; the lookup uses fileName+id.
    SessionMapObject s;
    s.storeCommentOut("lib.cpp", "C1", "// A class comment");
    EXPECT_EQ(RelaxCommentOut::apply(s, "proj", "lib.cpp", "class_specifier", "C1"),
              "// A class comment");
    // Different componentType string, same fileName+id → same result.
    EXPECT_EQ(RelaxCommentOut::apply(s, "proj", "lib.cpp", "struct_specifier", "C1"),
              "// A class comment");
}

TEST(relaxCommentOut_different_files_are_independent) {
    SessionMapObject s;
    s.storeCommentOut("a.cpp", "f1", "// comment A");
    s.storeCommentOut("b.cpp", "f1", "// comment B");
    EXPECT_EQ(RelaxCommentOut::apply(s, "p", "a.cpp", "function_definition", "f1"),
              "// comment A");
    EXPECT_EQ(RelaxCommentOut::apply(s, "p", "b.cpp", "function_definition", "f1"),
              "// comment B");
}

TEST(relaxCommentOut_multiline_comment_preserved) {
    SessionMapObject s;
    std::string block = "/**\n * @brief Sorts the list.\n * @param v The vector.\n */";
    s.storeCommentOut("sort.cpp", "f1", block);
    EXPECT_EQ(RelaxCommentOut::apply(s, "p", "sort.cpp", "function_definition", "f1"),
              block);
}

// ─── RelaxCompactBlock::apply ─────────────────────────────────────────────────

TEST(relaxCompactBlock_returns_original_name) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "processCustomerOrder");
    EXPECT_EQ(RelaxCompactBlock::apply(s, "proj", "order.cpp",
                                       "function_definition", "f1"),
              "processCustomerOrder");
}

TEST(relaxCompactBlock_returns_empty_for_unknown_compact_id) {
    // A newly added component has no mapping → return "" (keep compact id as-is).
    SessionMapObject s;
    EXPECT_EQ(RelaxCompactBlock::apply(s, "proj", "order.cpp",
                                       "function_definition", "f99"),
              "");
}

TEST(relaxCompactBlock_lookup_is_project_scoped) {
    SessionMapObject s;
    s.storeCompactBlock("projA", "f1", "funcInA");
    s.storeCompactBlock("projB", "f1", "funcInB");
    EXPECT_EQ(RelaxCompactBlock::apply(s, "projA", "x.cpp", "f", "f1"), "funcInA");
    EXPECT_EQ(RelaxCompactBlock::apply(s, "projB", "x.cpp", "f", "f1"), "funcInB");
}

TEST(relaxCompactBlock_works_for_class_symbols) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "C1", "ShoppingCart");
    EXPECT_EQ(RelaxCompactBlock::apply(s, "proj", "cart.cpp",
                                       "class_specifier", "C1"),
              "ShoppingCart");
}

TEST(relaxCompactBlock_works_for_variable_symbols) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "v1", "discountRate");
    EXPECT_EQ(RelaxCompactBlock::apply(s, "proj", "pricing.cpp", "v", "v1"),
              "discountRate");
}

TEST(relaxCompactBlock_fileName_is_not_part_of_lookup) {
    // CompactBlock lookup is project+compactId, fileName is ignored.
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "myFunc");
    EXPECT_EQ(RelaxCompactBlock::apply(s, "proj", "any_file.cpp", "f", "f1"),
              "myFunc");
    EXPECT_EQ(RelaxCompactBlock::apply(s, "proj", "other.cpp", "f", "f1"),
              "myFunc");
}

// ─── RelaxTextual::apply ──────────────────────────────────────────────────────

TEST(relaxTextual_no_markers_returns_unchanged) {
    SessionMapObject s;
    std::string text = "The function returned successfully.";
    EXPECT_EQ(RelaxTextual::apply(s, "proj", text), text);
}

TEST(relaxTextual_empty_input_returns_empty) {
    SessionMapObject s;
    EXPECT_EQ(RelaxTextual::apply(s, "proj", ""), "");
}

TEST(relaxTextual_single_known_marker_replaced) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "processCustomerOrder");
    std::string result = RelaxTextual::apply(s, "proj",
                                             "You should call {{f1}} to handle this.");
    EXPECT_EQ(result, "You should call processCustomerOrder to handle this.");
}

TEST(relaxTextual_multiple_known_markers_replaced) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "validateOrder");
    s.storeCompactBlock("proj", "C1", "OrderProcessor");
    std::string result = RelaxTextual::apply(s, "proj",
        "{{C1}} calls {{f1}} during checkout.");
    EXPECT_EQ(result, "OrderProcessor calls validateOrder during checkout.");
}

TEST(relaxTextual_unknown_marker_strips_braces_keeps_id) {
    SessionMapObject s;
    std::string result = RelaxTextual::apply(s, "proj",
                                             "Call {{f99}} to process.");
    // No mapping → strip {{ }}, keep compact id.
    EXPECT_EQ(result, "Call f99 to process.");
}

TEST(relaxTextual_mixed_known_and_unknown_markers) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "sendEmail");
    std::string result = RelaxTextual::apply(s, "proj",
        "Use {{f1}} and also {{f99}} for this.");
    EXPECT_EQ(result, "Use sendEmail and also f99 for this.");
}

TEST(relaxTextual_marker_at_start_of_string) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "C1", "PaymentGateway");
    EXPECT_EQ(RelaxTextual::apply(s, "proj", "{{C1}} handles payments."),
              "PaymentGateway handles payments.");
}

TEST(relaxTextual_marker_at_end_of_string) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "f2", "calculateTotal");
    EXPECT_EQ(RelaxTextual::apply(s, "proj", "The total is from {{f2}}"),
              "The total is from calculateTotal");
}

TEST(relaxTextual_adjacent_markers) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "init");
    s.storeCompactBlock("proj", "f2", "run");
    EXPECT_EQ(RelaxTextual::apply(s, "proj", "{{f1}}{{f2}}"),
              "initrun");
}

TEST(relaxTextual_repeated_same_marker) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "process");
    EXPECT_EQ(RelaxTextual::apply(s, "proj",
                                  "Call {{f1}}, then {{f1}}, then {{f1}}."),
              "Call process, then process, then process.");
}

TEST(relaxTextual_project_scoped_lookup) {
    SessionMapObject s;
    s.storeCompactBlock("projA", "f1", "funcFromA");
    s.storeCompactBlock("projB", "f1", "funcFromB");
    EXPECT_EQ(RelaxTextual::apply(s, "projA", "Use {{f1}}."),
              "Use funcFromA.");
    EXPECT_EQ(RelaxTextual::apply(s, "projB", "Use {{f1}}."),
              "Use funcFromB.");
}

TEST(relaxTextual_marker_with_class_and_function_symbols) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "C1", "OrderService");
    s.storeCompactBlock("proj", "f1", "createOrder");
    s.storeCompactBlock("proj", "v1", "orderCount");
    std::string result = RelaxTextual::apply(s, "proj",
        "{{C1}}.{{f1}}() increments {{v1}}.");
    EXPECT_EQ(result, "OrderService.createOrder() increments orderCount.");
}
