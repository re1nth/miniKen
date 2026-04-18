#include "goProfile.h"

#include <tree_sitter/api.h>

#ifdef HAVE_TREE_SITTER_GO
extern "C" { TSLanguage* tree_sitter_go(); }
#endif

LanguageProfile makeGoProfile() {
    LanguageProfile p;
    p.name = "go";

#ifndef HAVE_TREE_SITTER_GO
    p.grammar = nullptr;
    return p;
#else
    p.grammar = tree_sitter_go();

    p.definitionNodeTypes = {
        "function_declaration",   // func foo(...)
        "method_declaration",     // func (r T) foo(...)
        "type_declaration",       // type Foo struct{} / interface{}
        "var_declaration",        // var x int
        "const_declaration",      // const x = 1
        "import_declaration"      // import "fmt"
    };

    // In function_declaration the name is a direct [identifier] child.
    // In method_declaration the name is a direct [field_identifier] child.
    // In type/var/const declarations the name is inside a *_spec wrapper (below).
    p.identifierNodeTypes = {
        "identifier",
        "type_identifier",
        "field_identifier",
        "package_identifier"
    };

    // type_declaration  → type_spec  → type_identifier (name)
    // var_declaration   → var_spec   → identifier       (name)
    // const_declaration → const_spec → identifier       (name)
    p.declaratorWrappers = {
        "type_spec",
        "var_spec",
        "const_spec"
    };

    p.commentNodeTypes = {
        "comment"
    };

    p.componentTypeMap = {
        {"function_declaration", 'f'},
        {"method_declaration",   'f'},
        {"type_declaration",     'C'},
        {"import_declaration",   'p'}
        // var/const fall through to defaultComponentType = 'v'
    };

    p.defaultComponentType = 'v';
    return p;
#endif
}
