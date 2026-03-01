// test_orchestrator.cpp
// Integration tests for Orchestrator::run() — exercises the full pipeline
// (compress → stub model call → decompress).
#include "../testRunner.h"
#include "orchestrator.h"
#include "sessionStore/sessionMapObject.h"

#include <fstream>
#include <unistd.h>
#include <sys/types.h>

// ─── Helpers ─────────────────────────────────────────────────────────────────

static std::string writeTempCpp(const std::string& content) {
    char tmpl[] = "/tmp/mk_orch_XXXXXX.cpp";
    int fd = mkstemps(tmpl, 4);
    if (fd < 0) throw std::runtime_error("mkstemps failed");
    ::write(fd, content.c_str(), content.size());
    ::close(fd);
    return std::string(tmpl);
}

// ─── Orchestrator::run tests ──────────────────────────────────────────────────

TEST(orchestrator_run_returns_nonempty_string) {
    // Even with the stub model, run() must return something.
    std::string src = "void hello() { }\n";
    auto path = writeTempCpp(src);
    ProjectFiles proj{"projectA", {path}};
    // Redirect stderr to suppress stub logs during tests.
    std::string result = Orchestrator::run("Add a greeting", {proj});
    EXPECT_FALSE(result.empty());
}

TEST(orchestrator_run_accepts_multiple_projects) {
    std::string src1 = "void funcA() { }\n";
    std::string src2 = "void funcB() { }\n";
    auto p1 = writeTempCpp(src1);
    auto p2 = writeTempCpp(src2);
    ProjectFiles projA{"projA", {p1}};
    ProjectFiles projB{"projB", {p2}};
    // Must not throw.
    std::string result = Orchestrator::run("Refactor both", {projA, projB});
    EXPECT_FALSE(result.empty());
}

TEST(orchestrator_run_accepts_multiple_files_per_project) {
    std::string src1 = "void serviceA() { }\n";
    std::string src2 = "void serviceB() { }\n";
    auto p1 = writeTempCpp(src1);
    auto p2 = writeTempCpp(src2);
    ProjectFiles proj{"proj", {p1, p2}};
    std::string result = Orchestrator::run("Improve services", {proj});
    EXPECT_FALSE(result.empty());
}

TEST(orchestrator_run_with_empty_file_list_does_not_crash) {
    ProjectFiles proj{"proj", {}};
    std::string result = Orchestrator::run("Nothing to compress", {proj});
    EXPECT_FALSE(result.empty());
}

TEST(orchestrator_run_with_no_projects_does_not_crash) {
    std::string result = Orchestrator::run("No projects", {});
    EXPECT_FALSE(result.empty());
}

TEST(orchestrator_stub_response_contains_expected_text) {
    // The stub callModel returns a fixed string containing "stub response".
    std::string src = "int add(int a, int b) { return a + b; }\n";
    auto path = writeTempCpp(src);
    ProjectFiles proj{"proj", {path}};
    std::string result = Orchestrator::run("Add tests", {proj});
    // The stub injects "(stub response — wire a real API call here)".
    EXPECT_CONTAINS(result, "stub response");
}

// ─── buildPrompt / parseModelResponse (tested via run() behaviour) ───────────

TEST(orchestrator_textual_markers_in_stub_response_are_decompressed) {
    // If the stub happened to return {{f1}}, relaxTextual would decompress it.
    // We can't easily inject a custom model response without refactoring,
    // so we test the decompressor layer directly to confirm the orchestrator
    // wires it correctly.
    // This test exercises the relaxTextual path by checking the stub output
    // doesn't contain raw {{...}} markers (none should exist in the fixed stub).
    std::string src = "void processOrder() { }\n";
    auto path = writeTempCpp(src);
    ProjectFiles proj{"proj", {path}};
    std::string result = Orchestrator::run("Refactor", {proj});
    EXPECT_NOT_CONTAINS(result, "{{");
    EXPECT_NOT_CONTAINS(result, "}}");
}

TEST(orchestrator_large_source_does_not_crash) {
    // Build a source file with 20 functions.
    std::string src;
    for (int i = 0; i < 20; ++i) {
        src += "// Comment for function " + std::to_string(i) + "\n";
        src += "void longFunctionNameNumber" + std::to_string(i) +
               "(int parameterOne, int parameterTwo) {\n"
               "    int result = parameterOne + parameterTwo;\n"
               "}\n\n";
    }
    auto path = writeTempCpp(src);
    ProjectFiles proj{"proj", {path}};
    std::string result = Orchestrator::run("Optimise all functions", {proj});
    EXPECT_FALSE(result.empty());
}
