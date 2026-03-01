// test_parser.cpp
// Integration tests for QuerySquasher and ResponseBuilder using real C++ fixtures
// written to temp files at runtime (tree-sitter parses them).
#include "../testRunner.h"
#include "sessionStore/sessionMapObject.h"
#include "parser/querySquasher.h"
#include "parser/responseBuilder.h"
#include "parser/cppProfile.h"

#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Write content to a uniquely named temp file; returns the full path.
static std::string writeTempCpp(const std::string& content) {
    char tmpl[] = "/tmp/mk_test_XXXXXX.cpp";
    int fd = mkstemps(tmpl, 4);   // 4 = length of ".cpp"
    if (fd < 0) throw std::runtime_error("mkstemps failed");
    ::write(fd, content.c_str(), content.size());
    ::close(fd);
    return std::string(tmpl);
}

// ─── QuerySquasher tests ──────────────────────────────────────────────────────

TEST(querySquasher_returns_nonempty_output) {
    SessionMapObject s;
    std::string src = "int add(int a, int b) { return a + b; }\n";
    auto path = writeTempCpp(src);
    std::string out = QuerySquasher::apply(s, "proj", path, makeCppProfile());
    EXPECT_FALSE(out.empty());
}

TEST(querySquasher_function_name_is_compressed) {
    SessionMapObject s;
    std::string src =
        "int calculateTotalPrice(int unitPrice, int qty) {\n"
        "    return unitPrice * qty;\n"
        "}\n";
    auto path = writeTempCpp(src);
    std::string out = QuerySquasher::apply(s, "proj", path, makeCppProfile());
    // Original long name must not appear in the output.
    EXPECT_NOT_CONTAINS(out, "calculateTotalPrice");
    // A compact symbol must be present.
    EXPECT_CONTAINS(out, "f1");
}

TEST(querySquasher_session_maps_f1_to_function_name) {
    SessionMapObject s;
    std::string src = "void processOrder(int orderId) { }\n";
    auto path = writeTempCpp(src);
    QuerySquasher::apply(s, "proj", path, makeCppProfile());
    // The function should be registered as f1 in the session.
    EXPECT_EQ(s.getCompactBlockId("proj", "f1"), "processOrder");
}

TEST(querySquasher_class_name_is_compressed) {
    SessionMapObject s;
    std::string src = "class ShoppingCart { public: int itemCount; };\n";
    auto path = writeTempCpp(src);
    std::string out = QuerySquasher::apply(s, "proj", path, makeCppProfile());
    EXPECT_NOT_CONTAINS(out, "ShoppingCart");
    EXPECT_EQ(s.getCompactBlockId("proj", "C1"), "ShoppingCart");
}

TEST(querySquasher_inline_comment_stripped_from_output) {
    SessionMapObject s;
    std::string src =
        "// Computes the total price\n"
        "int computePrice(int price) { return price; }\n";
    auto path = writeTempCpp(src);
    std::string out = QuerySquasher::apply(s, "proj", path, makeCppProfile());
    EXPECT_NOT_CONTAINS(out, "Computes the total price");
    EXPECT_NOT_CONTAINS(out, "//");
}

TEST(querySquasher_comment_stored_in_session) {
    SessionMapObject s;
    std::string src =
        "// Sends an invoice to the customer\n"
        "void sendInvoice(int customerId) { }\n";
    auto path = writeTempCpp(src);
    // Need the basename to look up the comment later.
    std::string fileName = path.substr(path.rfind('/') + 1);
    QuerySquasher::apply(s, "proj", path, makeCppProfile());
    // The function is registered as f1; the comment should be stored under that key.
    std::string stored = s.getCommentOut(fileName, "f1");
    EXPECT_CONTAINS(stored, "Sends an invoice");
}

TEST(querySquasher_multiple_functions_get_sequential_ids) {
    SessionMapObject s;
    std::string src =
        "void alpha() { }\n"
        "void beta()  { }\n"
        "void gamma() { }\n";
    auto path = writeTempCpp(src);
    QuerySquasher::apply(s, "proj", path, makeCppProfile());
    // All three names must be registered.
    EXPECT_FALSE(s.getCompactBlockId("proj", "f1").empty());
    EXPECT_FALSE(s.getCompactBlockId("proj", "f2").empty());
    EXPECT_FALSE(s.getCompactBlockId("proj", "f3").empty());
    // And they must be distinct names.
    EXPECT_NE(s.getCompactBlockId("proj", "f1"), s.getCompactBlockId("proj", "f2"));
    EXPECT_NE(s.getCompactBlockId("proj", "f2"), s.getCompactBlockId("proj", "f3"));
}

TEST(querySquasher_identifier_used_multiple_times_maps_to_same_symbol) {
    SessionMapObject s;
    std::string src =
        "void helper() { }\n"
        "void main_fn() { helper(); helper(); helper(); }\n";
    auto path = writeTempCpp(src);
    std::string out = QuerySquasher::apply(s, "proj", path, makeCppProfile());
    // "helper" is registered as one compact id. Count occurrences in output.
    std::string helperCompact = s.allocateCompactId("proj", 'f', "helper");
    std::size_t count = 0, pos = 0;
    while ((pos = out.find(helperCompact, pos)) != std::string::npos) {
        ++count; pos += helperCompact.size();
    }
    // Should appear at least 3 times (definition + 3 calls).
    EXPECT_TRUE(count >= 3);
}

TEST(querySquasher_different_projects_have_independent_maps) {
    SessionMapObject s;
    std::string src = "void processOrder() { }\n";
    auto path1 = writeTempCpp(src);
    auto path2 = writeTempCpp(src);
    QuerySquasher::apply(s, "projA", path1, makeCppProfile());
    QuerySquasher::apply(s, "projB", path2, makeCppProfile());
    // Both projects register "processOrder" as f1 in their own namespace.
    EXPECT_EQ(s.getCompactBlockId("projA", "f1"), "processOrder");
    EXPECT_EQ(s.getCompactBlockId("projB", "f1"), "processOrder");
}

TEST(querySquasher_output_shorter_than_input) {
    SessionMapObject s;
    std::string src =
        "// Calculates the customer loyalty discount percentage\n"
        "float calculateCustomerLoyaltyDiscountPercentage(\n"
        "    CustomerAccount customerAccount, LoyaltyTier loyaltyTier) {\n"
        "    return customerAccount.years * loyaltyTier.multiplier;\n"
        "}\n";
    auto path = writeTempCpp(src);
    std::string out = QuerySquasher::apply(s, "proj", path, makeCppProfile());
    // Compressed output should be strictly shorter than the original.
    EXPECT_TRUE(out.size() < src.size());
}

// ─── ResponseBuilder tests ────────────────────────────────────────────────────

TEST(responseBuilder_restores_function_name) {
    // Set up a session with a known mapping, then decompress a snippet.
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "processCustomerOrder");
    // The compressed source uses f1 where the original had processCustomerOrder.
    std::string compressed = "void f1(int v1) { return; }\n";
    std::string out = ResponseBuilder::apply(s, "proj", compressed, makeCppProfile());
    EXPECT_CONTAINS(out, "processCustomerOrder");
    EXPECT_NOT_CONTAINS(out, "f1");
}

TEST(responseBuilder_restores_class_name) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "C1", "ShoppingCart");
    std::string compressed = "class C1 { public: int f1; };\n";
    std::string out = ResponseBuilder::apply(s, "proj", compressed, makeCppProfile());
    EXPECT_CONTAINS(out, "ShoppingCart");
}

TEST(responseBuilder_unknown_compact_id_kept_as_is) {
    // Newly added function (not in session) → keep compact id.
    SessionMapObject s;
    // Only f1 is mapped; f99 is not.
    s.storeCompactBlock("proj", "f1", "existingFunc");
    std::string compressed = "void f99(int x) { f1(); }\n";
    std::string out = ResponseBuilder::apply(s, "proj", compressed, makeCppProfile());
    // f1 should be restored, f99 should remain.
    EXPECT_CONTAINS(out, "existingFunc");
    EXPECT_CONTAINS(out, "f99");
}

TEST(responseBuilder_restores_comment_before_definition) {
    // Store the comment keyed by "compressed_response" (the synthetic filename
    // used internally by ResponseBuilder).
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "sendInvoice");
    s.storeCommentOut("compressed_response", "f1", "// Sends invoice to customer");
    std::string compressed = "void f1(int id) { }\n";
    std::string out = ResponseBuilder::apply(s, "proj", compressed, makeCppProfile());
    EXPECT_CONTAINS(out, "Sends invoice to customer");
    EXPECT_CONTAINS(out, "sendInvoice");
}

TEST(responseBuilder_multiple_symbols_all_restored) {
    SessionMapObject s;
    s.storeCompactBlock("proj", "f1", "validateOrder");
    s.storeCompactBlock("proj", "f2", "applyDiscount");
    s.storeCompactBlock("proj", "C1", "OrderManager");
    std::string compressed =
        "class C1 {\n"
        "    void f1();\n"
        "    void f2();\n"
        "};\n";
    std::string out = ResponseBuilder::apply(s, "proj", compressed, makeCppProfile());
    EXPECT_CONTAINS(out, "OrderManager");
    EXPECT_CONTAINS(out, "validateOrder");
    EXPECT_CONTAINS(out, "applyDiscount");
}

// ─── Round-trip: QuerySquasher → ResponseBuilder ──────────────────────────────

TEST(roundtrip_function_name_restored) {
    SessionMapObject s;
    std::string src = "void processPayment(int amount) { }\n";
    auto path = writeTempCpp(src);
    std::string compressed = QuerySquasher::apply(s, "proj", path, makeCppProfile());
    std::string restored   = ResponseBuilder::apply(s, "proj", compressed, makeCppProfile());
    EXPECT_CONTAINS(restored, "processPayment");
}

TEST(roundtrip_class_name_restored) {
    SessionMapObject s;
    std::string src = "class InvoiceGenerator { public: void generate(); };\n";
    auto path = writeTempCpp(src);
    std::string compressed = QuerySquasher::apply(s, "proj", path, makeCppProfile());
    std::string restored   = ResponseBuilder::apply(s, "proj", compressed, makeCppProfile());
    EXPECT_CONTAINS(restored, "InvoiceGenerator");
}

TEST(roundtrip_all_original_identifiers_present_in_restored) {
    SessionMapObject s;
    std::string src =
        "class CustomerDatabase {\n"
        "public:\n"
        "    void insertCustomer(int customerId);\n"
        "    void deleteCustomer(int customerId);\n"
        "    int  fetchCustomerById(int customerId);\n"
        "};\n";
    auto path = writeTempCpp(src);
    std::string compressed = QuerySquasher::apply(s, "proj", path, makeCppProfile());
    // Verify compression happened.
    EXPECT_NOT_CONTAINS(compressed, "CustomerDatabase");
    // Restore and verify all top-level names came back.
    std::string restored = ResponseBuilder::apply(s, "proj", compressed, makeCppProfile());
    EXPECT_CONTAINS(restored, "CustomerDatabase");
    EXPECT_CONTAINS(restored, "insertCustomer");
    EXPECT_CONTAINS(restored, "deleteCustomer");
    EXPECT_CONTAINS(restored, "fetchCustomerById");
}
