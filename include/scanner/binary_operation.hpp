#pragma once

#include "scanner/token_base.hpp"

namespace Scanner {

#define MACRO(name, type, literal, ...) name,
using BinaryOps = Pop<TTuple<
#include "ast/binop.dat"
struct Dummy
>>::Result;
#undef MACRO

template<typename T>
concept binop = Contains<BinaryOps, T>::value;

}