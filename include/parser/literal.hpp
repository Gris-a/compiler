#pragma once

#include "scanner/token.hpp"

#include <memory>

namespace Parser {

struct Expression;

#define MACRO(name, type, literal, ast, ...) \
struct ast {                                 \
    using Token = Scanner::name;             \
    type value;                              \
};

#include "tokens/literal.dat"
#undef MACRO

#define MACRO(name, type, literal, ast, ...) ast,
using Literals = Pop<TTuple<
#include "tokens/literal.dat"
struct Dummy
>>::Result;
#undef MACRO

template<typename T>
concept literal = Contains<Literals, T>::value;

}