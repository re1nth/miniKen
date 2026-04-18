#include "cProfile.h"

#include <tree_sitter/api.h>
extern "C" { TSLanguage* tree_sitter_c(); }

LanguageProfile makeCProfile() {
    LanguageProfile p;
    p.name    = "c";
    p.grammar = tree_sitter_c();

    p.definitionNodeTypes = {
        "function_definition",
        "struct_specifier",
        "enum_specifier",
        "type_definition",
        "declaration",
        "preproc_include"
    };

    p.identifierNodeTypes = {
        "identifier",
        "type_identifier",
        "field_identifier"
    };

    p.declaratorWrappers = {
        "function_declarator",
        "pointer_declarator"
    };

    p.commentNodeTypes = {
        "comment"
    };

    p.componentTypeMap = {
        {"function_definition", 'f'},
        {"struct_specifier",    'C'},
        {"enum_specifier",      'C'},
        {"preproc_include",     'p'}
    };

    p.defaultComponentType = 'v';
    return p;
}
