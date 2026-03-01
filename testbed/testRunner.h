#pragma once
// Minimal header-only test framework.
// Each test binary is a single .cpp that includes this header.
// TEST() macros self-register; main() at the bottom runs them all.

#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ─── Registry ────────────────────────────────────────────────────────────────

struct TestCase {
    std::string            name;
    std::function<void()>  fn;
};

inline std::vector<TestCase>& testRegistry() {
    static std::vector<TestCase> reg;
    return reg;
}

struct TestRegistrar {
    TestRegistrar(const char* name, std::function<void()> fn) {
        testRegistry().push_back({name, fn});
    }
};

// ─── Registration macro ───────────────────────────────────────────────────────

#define TEST(name)                                                        \
    static void _test_##name();                                           \
    static TestRegistrar _reg_##name(#name, _test_##name);               \
    static void _test_##name()

// ─── Assertion macros ─────────────────────────────────────────────────────────

#define EXPECT_TRUE(expr)                                                 \
    do {                                                                  \
        if (!(expr)) {                                                    \
            std::ostringstream _ss;                                       \
            _ss << __FILE__ << ":" << __LINE__                            \
                << "  EXPECT_TRUE(" #expr ") was false";                  \
            throw std::runtime_error(_ss.str());                          \
        }                                                                 \
    } while (0)

#define EXPECT_FALSE(expr)                                                \
    do {                                                                  \
        if ((expr)) {                                                     \
            std::ostringstream _ss;                                       \
            _ss << __FILE__ << ":" << __LINE__                            \
                << "  EXPECT_FALSE(" #expr ") was true";                  \
            throw std::runtime_error(_ss.str());                          \
        }                                                                 \
    } while (0)

#define EXPECT_EQ(a, b)                                                   \
    do {                                                                  \
        auto _a = (a);  auto _b = (b);                                   \
        if (!(_a == _b)) {                                                \
            std::ostringstream _ss;                                       \
            _ss << __FILE__ << ":" << __LINE__ << "  EXPECT_EQ failed\n" \
                << "    lhs = " << _a << "\n"                             \
                << "    rhs = " << _b;                                    \
            throw std::runtime_error(_ss.str());                          \
        }                                                                 \
    } while (0)

#define EXPECT_NE(a, b)                                                   \
    do {                                                                  \
        auto _a = (a);  auto _b = (b);                                   \
        if (_a == _b) {                                                   \
            std::ostringstream _ss;                                       \
            _ss << __FILE__ << ":" << __LINE__ << "  EXPECT_NE failed\n" \
                << "    both sides equal: " << _a;                        \
            throw std::runtime_error(_ss.str());                          \
        }                                                                 \
    } while (0)

#define EXPECT_CONTAINS(haystack, needle)                                 \
    do {                                                                  \
        std::string _h = (haystack), _n = (needle);                      \
        if (_h.find(_n) == std::string::npos) {                          \
            std::ostringstream _ss;                                       \
            _ss << __FILE__ << ":" << __LINE__                            \
                << "  EXPECT_CONTAINS failed\n"                           \
                << "    needle   : \"" << _n << "\"\n"                   \
                << "    haystack : \"" << _h << "\"";                    \
            throw std::runtime_error(_ss.str());                          \
        }                                                                 \
    } while (0)

#define EXPECT_NOT_CONTAINS(haystack, needle)                             \
    do {                                                                  \
        std::string _h = (haystack), _n = (needle);                      \
        if (_h.find(_n) != std::string::npos) {                          \
            std::ostringstream _ss;                                       \
            _ss << __FILE__ << ":" << __LINE__                            \
                << "  EXPECT_NOT_CONTAINS failed\n"                       \
                << "    found    : \"" << _n << "\"\n"                   \
                << "    in       : \"" << _h << "\"";                    \
            throw std::runtime_error(_ss.str());                          \
        }                                                                 \
    } while (0)

// ─── main ─────────────────────────────────────────────────────────────────────

int main() {
    int passed = 0, failed = 0;
    for (auto& tc : testRegistry()) {
        try {
            tc.fn();
            std::cout << "  PASS  " << tc.name << "\n";
            ++passed;
        } catch (std::exception& e) {
            std::cout << "  FAIL  " << tc.name << "\n"
                      << "        " << e.what() << "\n";
            ++failed;
        }
    }
    std::cout << "\n";
    if (failed == 0)
        std::cout << "All " << passed << " tests passed.\n";
    else
        std::cout << passed << " passed, " << failed << " FAILED.\n";
    return failed ? 1 : 0;
}
