#include "languageRegistry.h"

#include "profiles/cProfile.h"
#include "profiles/cppProfile.h"
#include "profiles/goProfile.h"
#include "profiles/pythonProfile.h"

#include <fstream>

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

static std::string extOf(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return {};
    return path.substr(dot);
}

static std::string dirOf(const std::string& path) {
    auto slash = path.rfind('/');
    if (slash == std::string::npos) return ".";
    return path.substr(0, slash);
}

static std::string stemOf(const std::string& path) {
    auto slash = path.rfind('/');
    std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
    auto dot = name.rfind('.');
    return (dot == std::string::npos) ? name : name.substr(0, dot);
}

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

LanguageRegistry& LanguageRegistry::instance() {
    static LanguageRegistry reg;
    return reg;
}

// ---------------------------------------------------------------------------
// init — register all built-in profiles
// ---------------------------------------------------------------------------

void LanguageRegistry::init() {
    static bool done = false;
    if (done) return;
    done = true;

    LanguageRegistry& reg = instance();

    // Reserve capacity for all built-in profiles upfront so that push_back
    // never reallocates owned_ mid-init (which would invalidate the pointers
    // already stored in byExt_ / byName_).
    reg.owned_.reserve(4);

    // C
    {
        LanguageProfile p = makeCProfile();
        if (p.grammar)
            reg.registerProfile(std::move(p), {".c"});
    }

    // C++ (also claims .h files when no paired .c exists — handled in forPath)
    {
        LanguageProfile p = makeCppProfile();
        if (p.grammar)
            reg.registerProfile(std::move(p), {".cpp", ".cc", ".cxx", ".hpp", ".hxx"});
    }

    // Go (only registered if HAVE_TREE_SITTER_GO was set at build time)
    {
        LanguageProfile p = makeGoProfile();
        if (p.grammar)
            reg.registerProfile(std::move(p), {".go"});
    }

    // Python (only registered if HAVE_TREE_SITTER_PYTHON was set at build time)
    {
        LanguageProfile p = makePythonProfile();
        if (p.grammar)
            reg.registerProfile(std::move(p), {".py"});
    }
}

// ---------------------------------------------------------------------------
// registerProfile
// ---------------------------------------------------------------------------

void LanguageRegistry::registerProfile(LanguageProfile profile,
                                        std::vector<std::string> extensions) {
    owned_.push_back(std::move(profile));
    const LanguageProfile* ptr = &owned_.back();

    byName_[ptr->name] = ptr;
    for (const auto& ext : extensions)
        byExt_[ext] = ptr;
}

// ---------------------------------------------------------------------------
// forPath
// ---------------------------------------------------------------------------

const LanguageProfile* LanguageRegistry::forPath(const std::string& filePath) const {
    std::string ext = extOf(filePath);

    // Special case: .h files — check whether a paired .c file exists on disk.
    // If yes, treat as C; otherwise fall through to the normal extension lookup (C++).
    if (ext == ".h") {
        std::string paired = dirOf(filePath) + "/" + stemOf(filePath) + ".c";
        std::ifstream probe(paired);
        if (probe.good()) {
            auto it = byName_.find("c");
            return (it != byName_.end()) ? it->second : nullptr;
        }
        // No paired .c file → treat as C++
        auto it = byName_.find("cpp");
        return (it != byName_.end()) ? it->second : nullptr;
    }

    auto it = byExt_.find(ext);
    return (it != byExt_.end()) ? it->second : nullptr;
}

// ---------------------------------------------------------------------------
// forName
// ---------------------------------------------------------------------------

const LanguageProfile* LanguageRegistry::forName(const std::string& name) const {
    auto it = byName_.find(name);
    return (it != byName_.end()) ? it->second : nullptr;
}
