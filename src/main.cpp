#include <iostream>

#include "CLI/CLI.hpp"

#include "scanner/scan.hpp"
#include "parser/parse.hpp"
#include "parser/print_vizitor.hpp"

CLI::App app{"compiler"};

std::string filename;

void setup_cli_parser() {
    app.add_option("source", filename)->required()->check(CLI::ExistingFile);
}

int main(int argc, char** argv) {
    setup_cli_parser();
    CLI11_PARSE(app, argc, argv);

    try {
        std::ifstream file(filename);
        Parser::Program program = Parser::parse(Scanner::scan(file));
        Parser::PrintVizitor vizitor;
        vizitor.vizit(program);

        std::ofstream out("dot.dot");
        out << vizitor.get_dot();

    } catch (Scanner::Position position) {
        std::cout << position.line << ' ' << position.pos << '\n';
    }
}