#include <concepts>
#include <fstream>
#include <iostream>
#include <type_traits>

#include "CLI/CLI.hpp"

#include "scanner/scan.hpp"
#include "parser/parse.hpp"
#include "semantic/semantic.hpp"

CLI::App app{"compiler"};

std::string filename;

void setup_cli_parser() {
    app.add_option("source", filename)->required()->check(CLI::ExistingFile);
}

void print_scope(const Parser::Semantic::ScopeNode &scope, int indent = 0) {
    std::string indent_prefix(indent * 2, ' ');
    std::cout << indent_prefix << "Scope: " << scope.get_name() << '\n';
    
    for (const auto &[name, variable] : scope.get_variables()) {
        std::cout << indent_prefix << "  var " << name << " : " << variable.type << '\n';
    }
    
    for (const auto &child : scope.get_children()) {
        print_scope(*child, indent + 1);
    }
}

int main(int argc, char** argv) {
    setup_cli_parser();
    CLI11_PARSE(app, argc, argv);

    try {
        std::ifstream file(filename);
        Parser::Program program = Parser::parse(Scanner::scan(file));
        Parser::Semantic::AnalysisResult analysis = Parser::Semantic::analyze(program);

        if (!analysis.diagnostics.empty()) {
            std::cout << "Semantic diagnostics:\n";
            for (const auto &message : analysis.diagnostics) {
                std::cout << "  - " << message << '\n';
            }
        } else {
            std::cout << "Semantic analysis passed.\n";
        }

        print_scope(*analysis.root_scope);

    } catch (Scanner::Position position) {
        std::cout << position.line << ' ' << position.pos << '\n';
    }
}