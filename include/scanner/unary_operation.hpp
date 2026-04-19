#pragma once

#include "scanner/token_base.hpp"

namespace Scanner {

#define MACRO(name, type, literal, ...) name,
using UnaryOps = Pop<TTuple<
#include "ast/unop.dat"
struct Dummy
>>::Result;
#undef MACRO

template<typename T>
concept unop = Contains<UnaryOps, T>::value;

}