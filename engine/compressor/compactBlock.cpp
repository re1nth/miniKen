#include "compactBlock.h"
#include <unordered_set>

bool CompactBlock::isPrimitive(const std::string& name) {
    static const std::unordered_set<std::string> kPrimitives = {
        // C / C++
        "void", "bool", "char", "wchar_t", "char8_t", "char16_t", "char32_t",
        "short", "int", "long", "float", "double",
        "signed", "unsigned", "auto", "nullptr",
        "size_t", "ptrdiff_t",
        "int8_t",  "int16_t",  "int32_t",  "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "int_fast8_t",  "int_fast16_t",  "int_fast32_t",  "int_fast64_t",
        "int_least8_t", "int_least16_t", "int_least32_t", "int_least64_t",
        "intmax_t", "uintmax_t", "intptr_t", "uintptr_t",
        // Java
        "byte", "boolean",
        // Python
        "str", "bytes", "list", "dict", "tuple", "set", "frozenset",
        // Go
        "rune", "any", "error", "uintptr",
        "int8",  "int16",  "int32",  "int64",
        "uint",  "uint8",  "uint16", "uint32", "uint64",
        "float32", "float64", "complex64", "complex128",
        "string",
        // JavaScript / TypeScript
        "number", "bigint", "symbol", "never", "unknown", "object", "undefined", "null",
        // Rust
        "i8",  "i16",  "i32",  "i64",  "i128",  "isize",
        "u8",  "u16",  "u32",  "u64",  "u128",  "usize",
        "f32", "f64",
    };
    return kPrimitives.count(name) > 0;
}

std::string CompactBlock::apply(SessionMapObject& session,
                                const std::string& projectId,
                                char componentType,
                                const std::string& originalName) {
    if (isPrimitive(originalName)) return originalName;
    return session.allocateCompactId(projectId, componentType, originalName);
}
