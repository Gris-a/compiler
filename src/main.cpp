#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "CLI/CLI.hpp"

#include "scanner/scan.hpp"
#include "parser/parse.hpp"
#include "parser/visitor/print_visitor.hpp"

CLI::App app{"compiler"};

std::string filename;
std::string output;

void setup_cli_parser() {
    app.add_option("source", filename)->required()->check(CLI::ExistingFile);
    app.add_option("-g, --graph-output", output, "PNG output filename");
}

int main(int argc, char **argv) {
    setup_cli_parser();
    CLI11_PARSE(app, argc, argv);

    try {
        std::ifstream file(filename);
        Parser::Program program = Parser::parse(Scanner::scan(file));

        if (!output.empty()) {
            const auto dot_path = std::filesystem::path(output).replace_extension(".dot");
            {
                std::ofstream dot_file(dot_path); 
                Parser::GraphvizPrinter(dot_file).visit(program);
            }

            std::ostringstream command;
            command << "dot -Tpng -o " << output << ' ' << dot_path.string();

            std::system(command.str().c_str());
        }

    } catch (Scanner::Position position) {
        std::cout << position.line << ' ' << position.pos << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
