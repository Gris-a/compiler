#pragma once

#include "keywords.hpp"
#include "literals.hpp"

namespace Scanner {

TOKEN(EOFToken, void, "");
#undef TOKEN

using Tokens = Concat<TTuple<EOFToken>, Concat<Keywords, Literals>::Result>::Result;

template<typename T>
concept token = type<T> || literal<T> || Contains<Tokens, T>::value;

using Token = TTupleVariant<Tokens>::Result;

inline bool valid_token(const Token &token) {
    return !std::holds_alternative<EOFToken>(token);
}

struct TokenInfo {
    TokenInfo(Position pos, Token token): pos(pos), token(token) {}
    Position pos;
    Token token;
};

};