#pragma once

#include "scanner/common/token_base.hpp"

namespace Scanner {

#define MACRO(name, type, literal, ...) name,
using Keywords = Pop<TTuple<
#include "tokens/keyword.dat"
struct Dummy
>>::Result;
#undef MACRO

template<typename T>
concept keyword = Contains<Keywords, T>::value;

}