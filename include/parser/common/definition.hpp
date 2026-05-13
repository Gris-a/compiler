#pragma once

#include "parser/common/statement.hpp"

namespace Parser {

struct Definition;

using DefinitionVariant = std::variant
< FunctionDefinition
, VariableDefinition
>;

struct Definition : DefinitionVariant {
    using variant::variant;
};

using Program = std::vector<Definition>;

};