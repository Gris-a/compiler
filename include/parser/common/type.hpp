#pragma once

#include "scanner/common/token.hpp"

#include <memory>

namespace Parser {

struct Type;

#define MACRO(name, type, literal, ast, ...) \
struct ast {                                 \
    using Token = Scanner::name;             \
};

#include "ast/type.dat"
#undef MACRO

#define MACRO(name, type, literal, ast, ...) ast,
using Types = Pop<TTuple<
#include "ast/type.dat"
struct Dummy
>>::Result;
#undef MACRO

template<typename T>
concept primary_type = Contains<Types, T>::value;

struct Pointer {
    std::unique_ptr<Type> type;
};

using TypeVariant = TTupleVariant
< Concat
  < Types
  , TTuple<Pointer>
  >::Result
>::Result;

struct Type : TypeVariant {
    using variant::variant;
};

}