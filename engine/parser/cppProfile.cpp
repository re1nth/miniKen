#include "cppProfile.h"

#include <tree_sitter/api.h>
extern "C" { TSLanguage* tree_sitter_cpp(); }

LanguageProfile makeCppProfile() {
    LanguageProfile p;
    p.name    = "cpp";
    p.grammar = tree_sitter_cpp();

    p.definitionNodeTypes = {
        "function_definition",
        "class_specifier",
        "struct_specifier",
        "declaration",
        "preproc_include",
        "namespace_definition"
    };

    p.identifierNodeTypes = {
        "identifier",
        "type_identifier",
        "field_identifier",
        "namespace_identifier"
    };

    p.declaratorWrappers = {
        "function_declarator",
        "pointer_declarator",
        "reference_declarator"
    };

    p.commentNodeTypes = {
        "comment"
    };

    p.componentTypeMap = {
        {"function_definition", 'f'},
        {"class_specifier",     'C'},
        {"struct_specifier",    'C'},
        {"preproc_include",     'p'}
    };

    p.defaultComponentType = 'v';
    return p;
}
