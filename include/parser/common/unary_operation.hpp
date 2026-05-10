#pragma once

#include "scanner/common/token.hpp"

#include <memory>

namespace Parser {

struct Expression;

struct UnaryOperationBase {
    std::unique_ptr<Expression> operand;
};

#define MACRO(name, type, literal, ast, ...) \
struct ast : UnaryOperationBase {            \
    using Token = Scanner::name;             \
};

#include "ast/unop.dat"
#undef MACRO

#define MACRO(name, type, literal, ast, ...) ast,
using UnaryOps = Pop<TTuple<
#include "ast/unop.dat"
struct Dummy
>>::Result;
#undef MACRO

template<typename T>
concept unop = Contains<UnaryOps, T>::value;

}