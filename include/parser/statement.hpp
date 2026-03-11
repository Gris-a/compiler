#pragma once

#include "parser/expression.hpp"
#include "parser/type.hpp"

namespace Parser {

struct Statement;

struct Variable {
    Identifier identifier;
    Type type;
};

struct VariableDefinition {
    Variable variable;
    std::unique_ptr<Expression> initializer;
};

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

using StatementVariant = std::variant
< Scope
, Condition
, Return
, Expression
, VariableDefinition
>;

struct Statement : StatementVariant {
    using variant::variant;
};

};