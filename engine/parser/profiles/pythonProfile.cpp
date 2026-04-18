#include "pythonProfile.h"

#include <tree_sitter/api.h>

#ifdef HAVE_TREE_SITTER_PYTHON
extern "C" { TSLanguage* tree_sitter_python(); }
#endif

LanguageProfile makePythonProfile() {
    LanguageProfile p;
    p.name = "python";

#ifndef HAVE_TREE_SITTER_PYTHON
    p.grammar = nullptr;
    return p;
#else
    p.grammar = tree_sitter_python();

    p.definitionNodeTypes = {
        "function_definition",      // def foo():
        "class_definition",         // class Foo:
        "decorated_definition",     // @decorator\ndef foo(): / class Foo:
        "import_statement",         // import os
        "import_from_statement",    // from os import path
        "assignment"                // module-level and class-body variable bindings
    };

    // All named identifiers in Python are [identifier] leaf nodes.
    p.identifierNodeTypes = {
        "identifier"
    };

    // decorated_definition wraps a function_definition or class_definition.
    //   extractName descends into function_definition → finds [identifier].
    // import_statement / import_from_statement wrap module names in dotted_name.
    //   extractName descends into dotted_name → finds first [identifier].
    p.declaratorWrappers = {
        "function_definition",
        "class_definition",
        "dotted_name"
    };

    p.commentNodeTypes = {
        "comment"
    };

    p.componentTypeMap = {
        {"function_definition",   'f'},
        {"decorated_definition",  'f'},
        {"class_definition",      'C'},
        {"import_statement",      'p'},
        {"import_from_statement", 'p'}
        // assignment falls through to defaultComponentType = 'v'
    };

    p.defaultComponentType = 'v';
    return p;
#endif
}
