#pragma once

#include "scanner/token_base.hpp"

namespace Scanner {

#define MACRO(name, type, literal, ...) name,
using Literals = Pop<TTuple<
#include "tokens/literal.dat"
struct Dummy
>>::Result;
#undef MACRO

template<typename T>
concept literal = Contains<Literals, T>::value;

}