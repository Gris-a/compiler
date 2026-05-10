#pragma once

#include "parser/common/expression.hpp"
#include "parser/common/type.hpp"

namespace Parser {

struct Statement;

struct Return {
    std::unique_ptr<Expression> expression;
};

struct Scope {
    std::vector<Statement> statements;
};

struct Condition {
    std::unique_ptr<Expression> if_expression;
    Scope statement;

    std::unique_ptr<Condition> else_statement;
};

struct Variable {
    Type type;
    Identifier identifier;
};

struct VariableDefinition {
    Variable variable;
    std::unique_ptr<Expression> initializer;
};

struct FunctionDeclaration {
    Identifier name;

    Type return_type;
    std::vector<Variable> arguments;
};

struct FunctionDefinition {
    FunctionDeclaration declaration;
    std::unique_ptr<Scope> definition;
};

using StatementVariant = std::variant
< Scope
, Condition
, Return
, Expression
, VariableDefinition
, FunctionDefinition
>;

struct Statement : StatementVariant {
    using variant::variant;
};

};