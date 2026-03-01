// generate.cpp
// Presentation driver: runs original.cpp through the engine and writes:
//   compressed.cpp   — what gets sent to the LLM
//   decompressed.cpp — what the user receives after the model responds
//
// Usage: ./generate_stages <path/to/original.cpp> <output_dir>

#include "sessionStore/sessionMapObject.h"
#include "parser/querySquasher.h"
#include "parser/responseBuilder.h"
#include "parser/cppProfile.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << content;
    std::cout << "  wrote  " << path
              << "  (" << content.size() << " bytes)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: generate_stages <original.cpp> <output_dir>\n";
        return 1;
    }

    const std::string inputPath  = argv[1];
    const std::string outputDir  = argv[2];
    const std::string projectId  = "demo";

    SessionMapObject session;

    // ── Stage 1: compress ────────────────────────────────────────────────────
    std::cout << "\n[Stage 1] Compressing " << inputPath << " ...\n";
    std::string compressed;
    try {
        compressed = QuerySquasher::apply(session, projectId, inputPath, makeCppProfile());
    } catch (std::exception& e) {
        std::cerr << "Compression failed: " << e.what() << "\n";
        return 1;
    }
    writeFile(outputDir + "/compressed.cpp", compressed);

    // ── Stage 2: decompress ──────────────────────────────────────────────────
    // In real usage the LLM would modify compressed.cpp and return it.
    // Here we pass the compressed output straight through ResponseBuilder
    // to show the round-trip decompression.
    std::cout << "\n[Stage 2] Decompressing ...\n";
    std::string decompressed;
    try {
        decompressed = ResponseBuilder::apply(session, projectId, compressed, makeCppProfile());
    } catch (std::exception& e) {
        std::cerr << "Decompression failed: " << e.what() << "\n";
        return 1;
    }
    writeFile(outputDir + "/decompressed.cpp", decompressed);

    // ── Summary ──────────────────────────────────────────────────────────────
    std::ifstream orig(inputPath, std::ios::binary | std::ios::ate);
    long origBytes = orig ? (long)orig.tellg() : -1;

    std::cout << "\n── Summary ────────────────────────────────────────────\n";
    std::cout << "  original.cpp    " << origBytes
              << " bytes\n";
    std::cout << "  compressed.cpp  " << compressed.size()
              << " bytes  ("
              << (100 - 100 * (int)compressed.size() / (origBytes > 0 ? origBytes : 1))
              << "% smaller)\n";
    std::cout << "  decompressed.cpp " << decompressed.size()
              << " bytes\n";
    std::cout << "────────────────────────────────────────────────────────\n\n";

    return 0;
}
