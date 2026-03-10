#pragma once

#include <vector>
#include <memory>
#include <variant>

namespace Parser {

struct Expression;

using Identifier = std::string;

struct IntegerLiteral {
    int value;
};

struct UnsignedLiteral {
    unsigned value;
};

struct BinaryOperationBase {
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

#define BINARY_OPERATION(name)  \
struct name : BinaryOperationBase {}

BINARY_OPERATION(Comma);

BINARY_OPERATION(Assign);

BINARY_OPERATION(Index);

BINARY_OPERATION(Addition);
BINARY_OPERATION(Subtraction);

BINARY_OPERATION(Division);
BINARY_OPERATION(Remainder);

BINARY_OPERATION(Multiplication);

BINARY_OPERATION(Equal);
BINARY_OPERATION(NotEqual);

BINARY_OPERATION(Less);
BINARY_OPERATION(LessEqual);

BINARY_OPERATION(Greater);
BINARY_OPERATION(GreaterEqual);

BINARY_OPERATION(OrLogical);
BINARY_OPERATION(AndLogical);

BINARY_OPERATION(Or);
BINARY_OPERATION(Xor);
BINARY_OPERATION(And);

BINARY_OPERATION(LeftShift);
BINARY_OPERATION(RightShift);

struct UnaryOperationBase {
    std::unique_ptr<Expression> operand;
};

#define UNARY_OPERATION(name)   \
struct name : UnaryOperationBase {}

UNARY_OPERATION(Reference);
UNARY_OPERATION(Dereference);

UNARY_OPERATION(Not);
UNARY_OPERATION(NotLogical);

UNARY_OPERATION(Plus);
UNARY_OPERATION(Minus);

UNARY_OPERATION(Increment);
UNARY_OPERATION(Decrement);

struct FunctionCall {
    std::unique_ptr<Expression> function;
    std::vector<Expression> arguments;
};

using ExpressionVariant = std::variant
< Identifier

, IntegerLiteral
, UnsignedLiteral

, Comma

, Assign

, Index

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

, Reference
, Dereference

, Not
, NotLogical

, Plus
, Minus

, Increment
, Decrement

, FunctionCall
>;

struct Expression : ExpressionVariant {
    using variant::variant;
};

};