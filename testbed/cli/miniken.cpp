// miniken — command-line driver for the miniKen token-compression engine.
//
// Usage:
//   miniken "your question here" file1.cpp [file2.cpp ...]
//
// Environment:
//   ANTHROPIC_API_KEY  — required for live API calls
//   MINIKEN_MODEL      — optional model override (default: claude-sonnet-4-6)

#include "orchestrator.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: miniken \"query\" file1.cpp [file2.cpp ...]\n";
        return 1;
    }

    std::string query = argv[1];

    ProjectFiles proj;
    proj.projectId = "default";
    for (int i = 2; i < argc; ++i)
        proj.codeFilePaths.push_back(argv[i]);

    std::string result = Orchestrator::run(query, {proj});

    std::cout << "\n=== miniKen Response ===\n" << result << "\n";
    return 0;
}
