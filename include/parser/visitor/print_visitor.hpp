#pragma once

#include "parser/definition.hpp"

namespace Parser {

struct GraphvizPrinter {
public:
    explicit GraphvizPrinter(std::ostream &output);

    size_t visit(const Program &program);

private:
    size_t visit(const Definition &definition);
    size_t visit(const VariableDefinition &variable_definition);
    size_t visit(const FunctionDeclaration &declaration);
    size_t visit(const Variable &variable);
    size_t visit(const Scope &scope);
    size_t visit(const Condition &condition);
    size_t visit(const Return &ret);
    size_t visit(const Statement &statement);
    size_t visit(const Expression &expression);
    size_t visit(const Type &type);

    size_t add_node(std::string label);
    void add_edge(size_t parent, size_t child);

    std::ostream &output_;
    size_t next_id_{0};
};

}