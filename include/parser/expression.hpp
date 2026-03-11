#pragma once

#include <vector>
#include <memory>
#include <variant>

#include "scanner/token.hpp"

namespace Parser {

struct Expression;

#define PRIMARY_EXPRESSION(name, token) \
struct name {                           \
    using Token = token;                \
    token::type value;                  \
}

PRIMARY_EXPRESSION(Identifier, Scanner::Identifier);

PRIMARY_EXPRESSION(Integer, Scanner::Integer);
PRIMARY_EXPRESSION(Unsigned, Scanner::Unsigned);

#undef PRIMARY_EXPRESSION

using PrimaryExpressions = TTuple<Identifier, Integer, Unsigned>;

template<typename T>
concept primary_expression = Contains<PrimaryExpressions, T>::value;

struct BinaryOperationBase {
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

#define BINARY_OPERATION(name, token)   \
struct name : BinaryOperationBase {     \
    using Token = token;                \
}

BINARY_OPERATION(Comma, Scanner::Comma);
BINARY_OPERATION(Assign, Scanner::Assign);

BINARY_OPERATION(Addition, Scanner::Plus);
BINARY_OPERATION(Subtraction, Scanner::Minus);

BINARY_OPERATION(Division, Scanner::Divide);
BINARY_OPERATION(Remainder, Scanner::Remainder);

BINARY_OPERATION(Multiplication, Scanner::Multiply);

BINARY_OPERATION(Equal, Scanner::Equal);
BINARY_OPERATION(NotEqual, Scanner::NotEqual);

BINARY_OPERATION(Less, Scanner::Less);
BINARY_OPERATION(LessEqual, Scanner::LessEqual);

BINARY_OPERATION(Greater, Scanner::Greater);
BINARY_OPERATION(GreaterEqual, Scanner::GreaterEqual);

BINARY_OPERATION(OrLogical, Scanner::OrLogical);
BINARY_OPERATION(AndLogical, Scanner::AndLogical);

BINARY_OPERATION(Or, Scanner::Or);
BINARY_OPERATION(Xor, Scanner::Xor);
BINARY_OPERATION(And, Scanner::And);

BINARY_OPERATION(LeftShift, Scanner::LeftShift);
BINARY_OPERATION(RightShift, Scanner::RightShift);

#undef BINARY_OPERATION

using BinaryOperations = TTuple
< Comma
, Assign

, Addition
, Subtraction

, Division
, Remainder

, Multiplication

, Equal
, NotEqual

, Less
, LessEqual

, Greater
, GreaterEqual

, OrLogical
, AndLogical

, Or
, Xor
, And

, LeftShift
, RightShift
>;

template<typename T>
concept binary_operation = Contains<BinaryOperations, T>::value;

struct UnaryOperationBase {
    std::unique_ptr<Expression> operand;
};

#define UNARY_OPERATION(name, token)    \
struct name : UnaryOperationBase {      \
    using Token = token;                \
}

UNARY_OPERATION(Reference, Scanner::Reference);
UNARY_OPERATION(Dereference, Scanner::Dereference);

UNARY_OPERATION(Not, Scanner::Not);
UNARY_OPERATION(NotLogical, Scanner::NotLogical);

UNARY_OPERATION(Plus, Scanner::Plus);
UNARY_OPERATION(Minus, Scanner::Minus);

UNARY_OPERATION(Increment, Scanner::Increment);
UNARY_OPERATION(Decrement, Scanner::Decrement);

#undef UNARY_OPERATION

using UnaryOperations = TTuple
< Reference
, Dereference

, Not
, NotLogical

, Plus
, Minus

, Increment
, Decrement
>;

template<typename T>
concept unary_operation = Contains<UnaryOperations, T>::value;

struct Index {
    std::unique_ptr<Expression> at;
    std::unique_ptr<Expression> pos;
};

struct FunctionCall {
    std::unique_ptr<Expression> function;
    std::vector<Expression> arguments;
};

using SpecialOperations = TTuple<Index, FunctionCall>;

using Operations = Concat
< Concat
    < BinaryOperations
    , UnaryOperations
    >::Result
, SpecialOperations
>::Result;

using ExpressionVariant = TTupleVariant<Concat<PrimaryExpressions, Operations>::Result>::Result;

struct Expression : ExpressionVariant {
    using variant::variant;
};

};