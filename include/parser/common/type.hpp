#pragma once
#include <memory>

#include "scanner/common/token.hpp"
#include "util/overloaded.hpp"
#include "util/hash.hpp"
#include "parser/common/ast_base.hpp"

namespace Parser {

struct Type;

#define MACRO(name, type, literal, ast, ...) \
struct ast : ASTBase {                       \
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

struct Pointer : ASTBase {
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

    friend bool operator==(const Type &lhs, const Type &rhs) {
        return std::visit(Overloaded {
            [&](const Pointer &l, const Pointer &r) {
                return *l.type == *r.type;
            },
            [&]([[maybe_unused]] const auto &l, [[maybe_unused]] const auto &r) {
                return (lhs.index() == rhs.index());
            }
        }, lhs, rhs);
    }

    struct Hash {
        std::size_t operator()(const Type& type) const {
            return std::visit(Overloaded {
                [](const Pointer &ptr) {
                    std::size_t seed = std::hash<std::size_t>{}(typeid(Pointer).hash_code());
                    hash_combine(seed, Hash{}(*ptr.type));
                    return seed;
                },
                [](const auto &t) {
                    return std::hash<std::size_t>{}(typeid(t).hash_code());
                }
            }, type);
        }
    };
};

}