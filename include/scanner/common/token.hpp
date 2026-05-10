#pragma once

#include "scanner/common/token_base.hpp"

#include "scanner/common/type.hpp"
#include "scanner/common/binary_operation.hpp"
#include "scanner/common/unary_operation.hpp"
#include "scanner/common/keyword.hpp"
#include "scanner/common/literal.hpp"

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