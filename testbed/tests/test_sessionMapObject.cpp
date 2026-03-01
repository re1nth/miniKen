// test_sessionMapObject.cpp
// Exhaustive unit tests for every public method of SessionMapObject.
#include "../testRunner.h"
#include "sessionStore/sessionMapObject.h"

// ─── sessionId ────────────────────────────────────────────────────────────────

TEST(sessionId_is_nonempty) {
    SessionMapObject s;
    EXPECT_TRUE(s.sessionId().size() >= 16);
}

TEST(sessionId_is_hex_digits) {
    SessionMapObject s;
    for (char c : s.sessionId())
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
}

TEST(sessionId_unique_across_instances) {
    SessionMapObject a, b;
    EXPECT_NE(a.sessionId(), b.sessionId());
}

// ─── storeCommentOut / getCommentOut ─────────────────────────────────────────

TEST(commentOut_store_and_retrieve) {
    SessionMapObject s;
    s.storeCommentOut("auth.cpp", "f1", "// Authenticates the user");
    EXPECT_EQ(s.getCommentOut("auth.cpp", "f1"), "// Authenticates the user");
}

TEST(commentOut_unknown_key_returns_empty_string) {
    SessionMapObject s;
    EXPECT_EQ(s.getCommentOut("auth.cpp", "f99"), "");
}

TEST(commentOut_empty_attachedId_is_valid_key) {
    SessionMapObject s;
    s.storeCommentOut("f.cpp", "", "orphan comment");
    EXPECT_EQ(s.getCommentOut("f.cpp", ""), "orphan comment");
}

TEST(commentOut_same_attachedId_different_files_are_independent) {
    SessionMapObject s;
    s.storeCommentOut("a.cpp", "f1", "comment-A");
    s.storeCommentOut("b.cpp", "f1", "comment-B");
    EXPECT_EQ(s.getCommentOut("a.cpp", "f1"), "comment-A");
    EXPECT_EQ(s.getCommentOut("b.cpp", "f1"), "comment-B");
}

TEST(commentOut_overwrite_replaces_existing_entry) {
    SessionMapObject s;
    s.storeCommentOut("x.cpp", "f1", "original");
    s.storeCommentOut("x.cpp", "f1", "updated");
    EXPECT_EQ(s.getCommentOut("x.cpp", "f1"), "updated");
}

TEST(commentOut_multiline_block_preserved) {
    SessionMapObject s;
    std::string block = "/**\n * Sorts the list.\n * @param list The list.\n */";
    s.storeCommentOut("sort.cpp", "f1", block);
    EXPECT_EQ(s.getCommentOut("sort.cpp", "f1"), block);
}

TEST(commentOut_multiple_entries_in_same_file) {
    SessionMapObject s;
    s.storeCommentOut("util.cpp", "f1", "// fn1 docs");
    s.storeCommentOut("util.cpp", "f2", "// fn2 docs");
    s.storeCommentOut("util.cpp", "C1", "// class docs");
    EXPECT_EQ(s.getCommentOut("util.cpp", "f1"), "// fn1 docs");
    EXPECT_EQ(s.getCommentOut("util.cpp", "f2"), "// fn2 docs");
    EXPECT_EQ(s.getCommentOut("util.cpp", "C1"), "// class docs");
}

// ─── storeCompactBlock / getCompactBlockId ────────────────────────────────────

TEST(compactBlock_fwd_lookup_returns_original_name) {
    SessionMapObject s;
    s.storeCompactBlock("proj1", "f1", "processCustomerOrder");
    EXPECT_EQ(s.getCompactBlockId("proj1", "f1"), "processCustomerOrder");
}

TEST(compactBlock_unknown_compact_id_returns_empty) {
    SessionMapObject s;
    EXPECT_EQ(s.getCompactBlockId("proj1", "f99"), "");
}

TEST(compactBlock_lookup_is_project_scoped) {
    SessionMapObject s;
    s.storeCompactBlock("projA", "f1", "funcInA");
    s.storeCompactBlock("projB", "f1", "funcInB");
    EXPECT_EQ(s.getCompactBlockId("projA", "f1"), "funcInA");
    EXPECT_EQ(s.getCompactBlockId("projB", "f1"), "funcInB");
    // projA's f1 should not bleed into projB's namespace.
    EXPECT_NE(s.getCompactBlockId("projA", "f1"), s.getCompactBlockId("projB", "f1"));
}

TEST(compactBlock_store_populates_reverse_map) {
    // After storeCompactBlock, allocateCompactId on the same originalName
    // must hit the fast path and return the same compact id.
    SessionMapObject s;
    s.storeCompactBlock("p", "f1", "myFunc");
    EXPECT_EQ(s.allocateCompactId("p", 'f', "myFunc"), "f1");
}

TEST(compactBlock_overwrite_updates_fwd_map) {
    SessionMapObject s;
    s.storeCompactBlock("p", "f1", "oldName");
    s.storeCompactBlock("p", "f1", "newName");
    EXPECT_EQ(s.getCompactBlockId("p", "f1"), "newName");
}

// ─── allocateCompactId ────────────────────────────────────────────────────────

TEST(allocate_function_prefix_is_f) {
    SessionMapObject s;
    std::string id = s.allocateCompactId("proj", 'f', "calculateTotalPrice");
    EXPECT_EQ(id.substr(0, 1), "f");
}

TEST(allocate_class_prefix_is_C) {
    SessionMapObject s;
    std::string id = s.allocateCompactId("proj", 'C', "ShoppingCart");
    EXPECT_EQ(id.substr(0, 1), "C");
}

TEST(allocate_variable_prefix_is_v) {
    SessionMapObject s;
    std::string id = s.allocateCompactId("proj", 'v', "discountRate");
    EXPECT_EQ(id.substr(0, 1), "v");
}

TEST(allocate_package_prefix_is_p) {
    SessionMapObject s;
    std::string id = s.allocateCompactId("proj", 'p', "iostream");
    EXPECT_EQ(id.substr(0, 1), "p");
}

TEST(allocate_first_of_each_type_is_numbered_1) {
    SessionMapObject s;
    EXPECT_EQ(s.allocateCompactId("proj", 'f', "alpha"),    "f1");
    EXPECT_EQ(s.allocateCompactId("proj", 'C', "Beta"),     "C1");
    EXPECT_EQ(s.allocateCompactId("proj", 'v', "gamma"),    "v1");
    EXPECT_EQ(s.allocateCompactId("proj", 'p', "delta"),    "p1");
}

TEST(allocate_increments_counter_per_prefix) {
    SessionMapObject s;
    EXPECT_EQ(s.allocateCompactId("proj", 'f', "fn1"), "f1");
    EXPECT_EQ(s.allocateCompactId("proj", 'f', "fn2"), "f2");
    EXPECT_EQ(s.allocateCompactId("proj", 'f', "fn3"), "f3");
}

TEST(allocate_deduplication_same_name_returns_same_id) {
    SessionMapObject s;
    std::string first  = s.allocateCompactId("proj", 'f', "processOrder");
    std::string second = s.allocateCompactId("proj", 'f', "processOrder");
    EXPECT_EQ(first, second);
}

TEST(allocate_deduplication_ignores_prefix_argument_on_revisit) {
    // Once stored as 'f1', querying with 'C' still returns 'f1' (fast path).
    SessionMapObject s;
    std::string id1 = s.allocateCompactId("proj", 'f', "myName");
    std::string id2 = s.allocateCompactId("proj", 'C', "myName");
    EXPECT_EQ(id1, id2);
}

TEST(allocate_different_names_get_different_ids) {
    SessionMapObject s;
    std::string a = s.allocateCompactId("proj", 'f', "funcA");
    std::string b = s.allocateCompactId("proj", 'f', "funcB");
    EXPECT_NE(a, b);
}

TEST(allocate_function_and_class_counters_are_independent) {
    SessionMapObject s;
    EXPECT_EQ(s.allocateCompactId("proj", 'f', "fn1"),  "f1");
    EXPECT_EQ(s.allocateCompactId("proj", 'C', "Cls1"), "C1");
    // Next function should be f2, not influenced by the class counter.
    EXPECT_EQ(s.allocateCompactId("proj", 'f', "fn2"),  "f2");
    EXPECT_EQ(s.allocateCompactId("proj", 'C', "Cls2"), "C2");
}

TEST(allocate_counters_are_project_scoped) {
    SessionMapObject s;
    // Each project starts its own counter from 1.
    s.allocateCompactId("projA", 'f', "shared");
    s.allocateCompactId("projB", 'f', "other");
    EXPECT_EQ(s.getCompactBlockId("projA", "f1"), "shared");
    EXPECT_EQ(s.getCompactBlockId("projB", "f1"), "other");
}

TEST(allocate_result_round_trips_through_fwd_lookup) {
    SessionMapObject s;
    std::string original = "processPaymentWithRetry";
    std::string compact  = s.allocateCompactId("proj", 'f', original);
    EXPECT_EQ(s.getCompactBlockId("proj", compact), original);
}
