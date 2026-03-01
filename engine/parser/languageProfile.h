#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <tree_sitter/api.h>

struct LanguageProfile {
    std::string  name;
    TSLanguage*  grammar;
    std::unordered_set<std::string> definitionNodeTypes;
    std::unordered_set<std::string> identifierNodeTypes;
    std::unordered_set<std::string> declaratorWrappers;    // nodes to descend into for name
    std::unordered_set<std::string> commentNodeTypes;
    std::unordered_map<std::string, char> componentTypeMap; // nodeType → 'f'|'C'|'v'|'p'
    char defaultComponentType = 'v';
};
