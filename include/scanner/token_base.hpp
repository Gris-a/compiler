#pragma once

#include <string_view>

#include "util/ttuple.hpp"

namespace Scanner {

template <typename T>
struct TockenContainer { T value; };

template <>
struct TockenContainer<void> {};

#define TOKEN(name, value_type, keyword)            \
struct name : TockenContainer<value_type> {         \
    using type = value_type;                        \
    static constexpr std::string_view key = keyword;\
}

struct Position {
    Position(size_t line, size_t pos): line(line), pos(pos) {}

    size_t line;
    size_t pos;
};

};