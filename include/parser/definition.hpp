#pragma once

#include "parser/statement.hpp"

namespace Parser {

struct Definition;

struct FunctionDeclaration {
    std::string name;

    Type return_type;
    std::vector<Variable> arguments;
};

struct FunctionDefinition {
    FunctionDeclaration declaration;
    std::unique_ptr<Scope> definition;
};

using DefinitionVariant = std::variant
< FunctionDefinition
, VariableDefinition
>;

struct Definition : DefinitionVariant {
    using variant::variant;
};

};