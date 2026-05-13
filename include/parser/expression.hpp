#pragma once

#include <vector>

#include "parser/binary_operation.hpp"
#include "parser/unary_operation.hpp"
#include "parser/literal.hpp"

namespace Parser {

struct Expression;

struct FunctionCall {
    std::unique_ptr<Expression> function;
    std::vector<Expression> arguments;
};

using ExpressionVariant = TTupleVariant
< Concat
  < Literals
  , BinaryOps
  , UnaryOps
  , TTuple<FunctionCall>
  >::Result
>::Result;

struct Expression : ExpressionVariant {
    using variant::variant;
};

}