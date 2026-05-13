#pragma once

#include "scanner/common/token_base.hpp"

namespace Scanner {

#define MACRO(name, type, literal, ...) name,
using Types = Pop<TTuple<
#include "ast/type.dat"
struct Dummy
>>::Result;
#undef MACRO

template<typename T>
concept type = Contains<Types, T>::value;

}