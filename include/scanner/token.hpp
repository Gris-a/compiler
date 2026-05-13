#pragma once

#include "scanner/token_base.hpp"

#include "scanner/type.hpp"
#include "scanner/binary_operation.hpp"
#include "scanner/unary_operation.hpp"
#include "scanner/keyword.hpp"
#include "scanner/literal.hpp"

namespace Scanner {

#define MACRO(name, type, literal, ...) name,
using Tokens = TTuple<
#include "tokens/keyword.dat"
#include "tokens/literal.dat"
EOFToken
>;
#undef MACRO

template<typename T>
concept token = std::same_as<T, EOFToken> || type<T> || binop<T> || unop<T> || literal<T> || keyword<T>;

using Token = TTupleVariant<Tokens>::Result;

inline bool valid_token(const Token &token) {
    return !std::holds_alternative<EOFToken>(token);
}

struct TokenInfo {
    TokenInfo(Position pos, Token token): pos(pos), token(token) {}
    Position pos;
    Token token;
};

}