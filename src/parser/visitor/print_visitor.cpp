#include "parser/visitor/print_visitor.hpp"
#include "util/overloaded.hpp"

#include <sstream>
#include <string_view>

namespace Parser {

namespace {
    
std::string escape_label(std::string_view value) {
    std::string result;
    result.reserve(value.size() * 2);

    for (char c : value) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            default: result += c;
        }
    }

    return result;
}

template<typename T>
std::string value_to_string(const T &value) {
    if constexpr (std::same_as<std::decay_t<T>, std::string>) {
        return value;
    } else {
        std::ostringstream out;
        out << value;
        return out.str();
    }
}

template<typename T>
std::string token_label() {
    using Node = std::decay_t<T>;
    using Token = typename Node::Token;
    return std::string(Token::key);
}

}

GraphvizPrinter::GraphvizPrinter(std::ostream &output): output_(output) {}

size_t GraphvizPrinter::visit(const Program &program) {
    output_ << "digraph AST {\n";
    output_ << "  rankdir=TB;\n";
    output_ << "  node [shape=box, fontname=\"Courier\"];\n";

    size_t root = add_node("Program");
    for (const Definition &definition : program) {
        size_t child = visit(definition);
        add_edge(root, child);
    }

    output_ << "}\n";
    return root;
}

size_t GraphvizPrinter::visit(const Definition &definition) {
    return std::visit(Overloaded {
        [this](const FunctionDefinition &function_definition) {
            size_t id = add_node("FunctionDefinition");
            size_t decl_id = visit(function_definition.declaration);
            add_edge(id, decl_id);
            if (function_definition.definition) {
                size_t body_id = visit(*function_definition.definition);
                add_edge(id, body_id);
            }
            return id;
        },
        [this](const VariableDefinition &variable_definition) {
            return visit(variable_definition);
        }
    }, definition);
}

size_t GraphvizPrinter::visit(const VariableDefinition &variable_definition) {
    size_t id = add_node("VariableDefinition");
    size_t variable_id = visit(variable_definition.variable);
    add_edge(id, variable_id);
    if (variable_definition.initializer) {
        size_t init_id = visit(*variable_definition.initializer);
        add_edge(id, init_id);
    }
    return id;
}

size_t GraphvizPrinter::visit(const FunctionDeclaration &declaration) {
    size_t id = add_node("FunctionDeclaration");
    size_t name_id = add_node(std::string("Identifier: ") + declaration.name.value);
    add_edge(id, name_id);
    size_t return_type_id = visit(declaration.return_type);
    add_edge(id, return_type_id);
    if (!declaration.arguments.empty()) {
        size_t args_id = add_node("Arguments");
        add_edge(id, args_id);
        for (const Variable &argument : declaration.arguments) {
            size_t arg_id = visit(argument);
            add_edge(args_id, arg_id);
        }
    }
    return id;
}

size_t GraphvizPrinter::visit(const Variable &variable) {
    size_t id = add_node("Variable");
    size_t type_id = visit(variable.type);
    add_edge(id, type_id);
    size_t name_id = add_node(std::string("Identifier: ") + variable.identifier.value);
    add_edge(id, name_id);
    return id;
}

size_t GraphvizPrinter::visit(const Scope &scope) {
    size_t id = add_node("Scope");
    for (const Statement &statement : scope.statements) {
        size_t child = visit(statement);
        add_edge(id, child);
    }
    return id;
}

size_t GraphvizPrinter::visit(const Condition &condition) {
    size_t id = add_node("Condition");
    size_t if_id = add_node("IfExpression");
    add_edge(id, if_id);
    add_edge(if_id, visit(*condition.if_expression));

    size_t then_id = add_node("Then");
    add_edge(id, then_id);
    add_edge(then_id, visit(condition.statement));

    if (condition.else_statement) {
        size_t else_id = add_node("Else");
        add_edge(id, else_id);
        add_edge(else_id, visit(*condition.else_statement));
    }
    return id;
}

size_t GraphvizPrinter::visit(const Return &ret) {
    size_t id = add_node("Return");
    if (ret.expression) {
        size_t expr_id = visit(*ret.expression);
        add_edge(id, expr_id);
    }
    return id;
}

size_t GraphvizPrinter::visit(const Statement &statement) {
    return std::visit(Overloaded {
        [this](const Scope &scope) { return visit(scope); },
        [this](const Condition &condition) { return visit(condition); },
        [this](const Return &ret) { return visit(ret); },
        [this](const Expression &expression) {
            size_t id = add_node("Expression");
            size_t child = visit(expression);
            add_edge(id, child);
            return id;
        },
        [this](const VariableDefinition &variable_definition) {
            return visit(variable_definition);
        }
    }, statement);
}

size_t GraphvizPrinter::visit(const Expression &expression) {
    return std::visit(Overloaded {
        [this](const FunctionCall &call) {
            size_t id = add_node("FunctionCall");
            size_t function_id = add_node("Target");
            add_edge(id, function_id);
            add_edge(function_id, visit(*call.function));
            if (!call.arguments.empty()) {
                size_t args_id = add_node("Arguments");
                add_edge(id, args_id);
                for (const Expression &argument : call.arguments) {
                    size_t arg_id = visit(argument);
                    add_edge(args_id, arg_id);
                }
            }
            return id;
        },
        [this](const auto &node) -> size_t {
            using Node = std::decay_t<decltype(node)>;
            if constexpr (Parser::literal<Node>) {
                return add_node(token_label<Node>() + ": " + value_to_string(node.value));
            } else if constexpr (Parser::binop<Node>) {
                size_t id = add_node(std::string("Binary: ") + token_label<Node>());
                add_edge(id, visit(*node.lhs));
                add_edge(id, visit(*node.rhs));
                return id;
            } else if constexpr (Parser::unop<Node>) {
                size_t id = add_node(std::string("Unary: ") + token_label<Node>());
                add_edge(id, visit(*node.operand));
                return id;
            } else {
                static_assert(!sizeof(Node), "Unsupported Expression variant in GraphvizPrinter");
            }
        }
    }, expression);
}

size_t GraphvizPrinter::visit(const Type &type) {
    return std::visit(Overloaded {
        [this](const Pointer &pointer) {
            size_t id = add_node("Pointer");
            add_edge(id, visit(*pointer.type));
            return id;
        },
        [this](const auto &node) -> size_t {
            using Node = std::decay_t<decltype(node)>;
            return add_node(std::string("Type: ") + token_label<Node>());
        }
    }, type);
}

size_t GraphvizPrinter::add_node(std::string label) {
    size_t id = next_id_++;
    output_ << " node" << id << " [label=\"" << escape_label(label) << "\", shape=box, fontname=\"Courier\"]\n";
    return id;
}

void GraphvizPrinter::add_edge(size_t parent, size_t child) {
    output_ << " node" << parent << " -> node" << child << "\n";
}

}