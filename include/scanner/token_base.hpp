#pragma once

#include <string_view>

#include <string>
#include <inttypes.h>

#include "util/ttuple.hpp"

namespace Scanner {

template<typename T>
struct TokenContainer { T value; };

template<>
struct TokenContainer<void> {};

struct Position {
    Position(size_t line, size_t pos): line(line), pos(pos) {}
    size_t line, pos;
};

#define MACRO(name, value_type, literal, ...)        \
struct name : TokenContainer<value_type> {           \
    using type = value_type;                         \
    static constexpr std::string_view key = literal; \
};

#include "tokens/keyword.dat"
#include "tokens/literal.dat"

MACRO(EOFToken, void, "");
#undef MACRO

}