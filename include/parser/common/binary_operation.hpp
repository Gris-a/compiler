#pragma once
#include <memory>

#include "scanner/common/token.hpp"
#include "parser/common/ast_base.hpp"

namespace Parser {

struct Expression;

struct BinaryOperationBase : ASTBase {
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

#define MACRO(name, type, literal, ast, ...) \
struct ast : BinaryOperationBase {           \
    using Token = Scanner::name;             \
};

#include "ast/binop.dat"
#undef MACRO

#define MACRO(name, type, literal, ast, ...) ast,
using BinaryOps = Pop<TTuple<
#include "ast/binop.dat"
struct Dummy
>>::Result;
#undef MACRO

template<typename T>
concept binop = Contains<BinaryOps, T>::value;

}