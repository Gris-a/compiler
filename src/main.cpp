#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "CLI/CLI.hpp"

#include "scanner/scan.hpp"
#include "parser/parse.hpp"
#include "parser/semantics.hpp"

CLI::App app{"compiler"};

std::string filename;

void setup_cli_parser() {
    app.add_option("source", filename)->required()->check(CLI::ExistingFile);
}

int main(int argc, char **argv) {
    setup_cli_parser();
    CLI11_PARSE(app, argc, argv);

    try {
        std::ifstream file(filename);
        Parser::Program program = Parser::parse(Scanner::scan(file));
        Parser::Semantic semantic(program);

        for (const auto &issue: semantic.issues()) {
            auto pos = issue.info().pos;
            std::cout << (issue.severity() == Parser::Semantic::Issue::Severity::Error ? "Error: " : "Warning: ")
                      << issue.message() 
                      << " at " << pos.line << ' ' << pos.pos << '\n';

        }
    } catch (Scanner::Position position) {
        std::cout << "Error: "
                  << "invalid syntax"
                  << " at " << position.line << ' ' << position.pos << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}