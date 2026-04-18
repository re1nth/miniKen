#pragma once
#include "languageProfile.h"

#include <string>
#include <unordered_map>
#include <vector>

// Singleton registry that owns all LanguageProfile instances and maps
// file extensions / language names to them.
//
// Usage:
//   LanguageRegistry::init();                         // once at startup
//   const LanguageProfile* p = LanguageRegistry::instance().forPath(path);
//   if (!p) { /* pass file through uncompressed */ }
class LanguageRegistry {
public:
    static LanguageRegistry& instance();

    // Register all built-in profiles. Call once before any forPath()/forName().
    // Profiles whose grammar pointer is nullptr (grammar not installed) are skipped.
    static void init();

    // Register a profile for one or more file extensions (e.g. {".go", ".Go"}).
    // Takes ownership of the profile by value.
    void registerProfile(LanguageProfile profile, std::vector<std::string> extensions);

    // Returns the profile for the given file path (uses file extension + optional
    // filesystem probe for .h files). Returns nullptr if the language is unknown.
    const LanguageProfile* forPath(const std::string& filePath) const;

    // Returns the profile whose name matches the stored session language string.
    // Used during decompression. Returns nullptr if name is unknown/empty.
    const LanguageProfile* forName(const std::string& name) const;

private:
    LanguageRegistry() = default;

    std::vector<LanguageProfile>                            owned_;
    std::unordered_map<std::string, const LanguageProfile*> byExt_;
    std::unordered_map<std::string, const LanguageProfile*> byName_;
};
