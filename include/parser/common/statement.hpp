#pragma once

#include "parser/common/expression.hpp"
#include "parser/common/type.hpp"

namespace Parser {

struct Statement;

struct Return : ASTBase {
    std::unique_ptr<Expression> expression;
};

struct Scope : ASTBase {
    std::vector<Statement> statements;
};

struct Condition : ASTBase {
    std::unique_ptr<Expression> if_expression;
    Scope statement;

    std::unique_ptr<Condition> else_statement;
};

struct Variable {
    Type type;
    Identifier identifier;
};

struct VariableDefinition : ASTBase {
    Variable variable;
    std::unique_ptr<Expression> initializer;
};

struct FunctionDeclaration : ASTBase {
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