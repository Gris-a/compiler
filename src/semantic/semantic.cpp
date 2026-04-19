#include "semantic/semantic.hpp"
#include "util/overloaded.hpp"

#include <algorithm>
#include <iostream>

namespace Parser::Semantic {

ScopeNode::ScopeNode(std::string name, std::shared_ptr<SymbolTable> symbol_table)
    : name_(std::move(name))
    , symbol_table_(std::move(symbol_table))
{}

std::unique_ptr<ScopeNode> ScopeNode::create_root(std::shared_ptr<SymbolTable> symbol_table) {
    return std::unique_ptr<ScopeNode>(new ScopeNode("global", std::move(symbol_table)));
}

ScopeNode *ScopeNode::add_child(std::string child_name) {
    children_.emplace_back(std::unique_ptr<ScopeNode>(new ScopeNode(std::move(child_name), symbol_table_)));
    auto *child = children_.back().get();
    child->parent_ = this;
    return child;
}

const std::string &ScopeNode::get_name() const {
    return name_;
}

ScopeNode *ScopeNode::get_parent() const {
    return parent_;
}

const std::vector<std::unique_ptr<ScopeNode>> &ScopeNode::get_children() const {
    return children_;
}

const std::unordered_map<std::string, VariableSymbol> &ScopeNode::get_variables() const {
    return variables_;
}

const VariableSymbol *ScopeNode::find_variable(const std::string &name) const {
    auto it = variables_.find(name);
    if (it != variables_.end()) {
        return &it->second;
    }

    if (parent_) {
        return parent_->find_variable(name);
    }

    return nullptr;
}

const VariableSymbol *ScopeNode::find_variable_in_parent(const std::string &name) const {
    if (!parent_) {
        return nullptr;
    }
    return parent_->find_variable(name);
}

bool ScopeNode::has_variable(const std::string &name) const {
    return variables_.contains(name);
}

VariableSymbol *ScopeNode::add_variable(const std::string &name, VariableSymbol symbol) {
    auto [it, inserted] = variables_.emplace(name, std::move(symbol));
    return inserted ? &it->second : nullptr;
}

static std::string describe_type(const Type &type);

template<typename Identifier>
static std::string identifier_name(const Identifier &identifier) {
    return identifier.value;
}

struct Analyzer {
    AnalysisResult &result;
    ScopeNode *current_scope;

    explicit Analyzer(AnalysisResult &result)
        : result(result)
        , current_scope(result.root_scope.get())
    {}

    void analyze(const Program &program) {
        register_functions(program);
        for (const auto &definition : program) {
            visit_definition(definition);
        }
    }

    void register_functions(const Program &program) {
        for (const auto &definition : program) {
            std::visit(
                Overloaded {
                    [&](const FunctionDefinition &function) {
                        auto name = identifier_name(function.declaration.name);
                        if (result.symbol_table->functions.contains(name)) {
                            result.diagnostics.emplace_back("Function '" + name + "' redeclared in the global scope");
                            return;
                        }
                        FunctionSymbol symbol;
                        symbol.name = name;
                        symbol.return_type = describe_type(function.declaration.return_type);
                        for (const auto &argument : function.declaration.arguments) {
                            symbol.arguments.push_back({identifier_name(argument.identifier), describe_type(argument.type), true, false});
                        }
                        result.symbol_table->functions.emplace(name, std::move(symbol));
                    },
                    [&](const VariableDefinition &variable_definition) {
                        auto name = identifier_name(variable_definition.variable.identifier);
                        if (result.symbol_table->variables.contains(name)) {
                            result.diagnostics.emplace_back("Global variable '" + name + "' redeclared in the global scope");
                            return;
                        }
                        VariableSymbol symbol{name, describe_type(variable_definition.variable.type), false, false};
                        result.symbol_table->variables.emplace(name, std::move(symbol));
                    }
                },
                definition
            );
        }
    }

    void visit_definition(const Definition &definition) {
        std::visit(
            Overloaded {
                [&](const FunctionDefinition &function) {
                    const auto name = identifier_name(function.declaration.name);
                    ScopeNode *function_scope = current_scope->add_child("function " + name);
                    ScopeNode *previous_scope = current_scope;
                    current_scope = function_scope;

                    for (const auto &argument : function.declaration.arguments) {
                        declare_variable(argument.identifier, argument.type, true);
                    }

                    visit_scope(*function.definition);
                    current_scope = previous_scope;
                },
                [&](const VariableDefinition &variable_definition) {
                    declare_variable(variable_definition.variable.identifier, variable_definition.variable.type, false);
                    if (variable_definition.initializer) {
                        visit_expression(*variable_definition.initializer);
                    }
                }
            },
            definition
        );
    }

    void visit_statements(const std::vector<Statement> &statements) {
        for (const auto &statement : statements) {
            visit_statement(statement);
        }
    }

    void visit_scope(const Scope &scope) {
        ScopeNode *inner_scope = current_scope->add_child("block");
        ScopeNode *previous_scope = current_scope;
        current_scope = inner_scope;

        visit_statements(scope.statements);
        current_scope = previous_scope;
    }

    void visit_condition(const Condition &condition) {
        visit_expression(*condition.if_expression);
        visit_scope(condition.statement);
        if (condition.else_statement) {
            visit_condition(*condition.else_statement);
        }
    }

    void visit_statement(const Statement &statement) {
        std::visit(
            Overloaded {
                [&](const Scope &scope) {
                    visit_scope(scope);
                },
                [&](const Condition &condition) {
                    visit_condition(condition);
                },
                [&](const Return &ret) {
                    if (ret.expression) {
                        visit_expression(*ret.expression);
                    }
                },
                [&](const Expression &expression) {
                    visit_expression(expression);
                },
                [&](const VariableDefinition &variable_definition) {
                    declare_variable(variable_definition.variable.identifier, variable_definition.variable.type, false);
                    if (variable_definition.initializer) {
                        visit_expression(*variable_definition.initializer);
                    }
                }
            },
            statement
        );
    }

    void visit_expression(const Expression &expression) {
        std::visit(
            Overloaded {
                [&](const Identifier &identifier) {
                    resolve_identifier(identifier);
                },
                [&](const Integer &) {
                    // integer literal does not require semantic lookup
                },
                [&](const FunctionCall &function_call) {
                    visit_expression(*function_call.function);
                    for (const auto &argument : function_call.arguments) {
                        visit_expression(argument);
                    }
                },
                [&](const BinaryOperationBase &binary) {
                    visit_expression(*binary.lhs);
                    visit_expression(*binary.rhs);
                },
                [&](const UnaryOperationBase &unary) {
                    visit_expression(*unary.operand);
                }
            },
            static_cast<const ExpressionVariant &>(expression)
        );
    }

    template<typename Identifier>
    const VariableSymbol *declare_variable(const Identifier &identifier, const Type &type, bool is_argument) {
        std::string name = identifier_name(identifier);
        if (current_scope->has_variable(name)) {
            result.diagnostics.emplace_back("Variable '" + name + "' is already declared in this scope");
            return nullptr;
        }

        if (auto *outer = current_scope->find_variable_in_parent(name)) {
            result.diagnostics.emplace_back("Variable '" + name + "' shadows a variable from an outer scope");
        }

        VariableSymbol symbol{std::move(name), describe_type(type), is_argument, false};
        return current_scope->add_variable(symbol.name, std::move(symbol));
    }

    Resolution resolve_identifier(const Identifier &identifier) {
        std::string name = identifier_name(identifier);

        if (const auto *variable = current_scope->find_variable(name)) {
            return Resolution{ReferenceKind::Variable, name, variable, nullptr};
        }

        if (auto it = result.symbol_table->functions.find(name); it != result.symbol_table->functions.end()) {
            return Resolution{ReferenceKind::Function, name, nullptr, &it->second};
        }

        result.diagnostics.emplace_back("Identifier '" + name + "' is used before declaration");
        return Resolution{ReferenceKind::Unknown, std::move(name), nullptr, nullptr};
    }
};

AnalysisResult analyze(const Program &program) {
    AnalysisResult result;
    result.symbol_table = std::make_shared<SymbolTable>();
    result.root_scope = ScopeNode::create_root(result.symbol_table);

    Analyzer analyzer(result);
    analyzer.analyze(program);

    return result;
}

static std::string describe_type(const Type &type) {
    return std::visit(
        Overloaded {
            [](const VoidType &) {
                return std::string("void");
            },
            [](const IntegerType &) {
                return std::string("int");
            },
            [&](const Pointer &pointer) {
                return describe_type(*pointer.type) + "*";
            }
        },
        static_cast<const TypeVariant &>(type)
    );
}

}