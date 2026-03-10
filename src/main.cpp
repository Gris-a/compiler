#include <iostream>

#include "CLI/CLI.hpp"

#include "scanner/scan.hpp"
#include "parser/parse.hpp"

CLI::App app{"compiler"};

std::string filename;

void setup_cli_parser() {
    app.add_option("source", filename)->required()->check(CLI::ExistingFile);
}

int main(int argc, char** argv) {
    setup_cli_parser();
    CLI11_PARSE(app, argc, argv);

    std::ifstream file(filename);
    auto tokens = Scanner::scan(file);
    
    auto definitions = Parser::parse(tokens);
}