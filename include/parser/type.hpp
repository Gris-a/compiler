#pragma once

#include <variant>
#include <memory>

namespace Parser {

struct Type;

struct VoidType {};
struct IntegerType {};
struct UnsignedType {};

struct Pointer {
    std::unique_ptr<Type> type;
};

using TypeVariant = std::variant
< VoidType

, IntegerType
, UnsignedType

, Pointer
>;

struct Type : TypeVariant {
    using variant::variant;
};

};