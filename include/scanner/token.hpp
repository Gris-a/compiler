#pragma once

#include "keywords.hpp"
#include "literals.hpp"

namespace Scanner {

TOKEN(NoToken, void, "");

using Tokens = Concat<Keywords, Literals>::Result;

template<typename T>
concept token = Contains<Tokens, T>::value;

using Types = TTuple<VoidType, IntegerType, UnsignedType>;

template<typename T>
concept type = Contains<Types, T>::value;

using Token = TTupleVariant<Concat<TTuple<NoToken>, Tokens>::Result>::Result;

inline bool valid_token(const Token &token) {
    return !std::holds_alternative<NoToken>(token);
}

struct TokenInfo {
    TokenInfo(Position pos, Token token): pos(pos), token(token) {}
    Position pos;
    Token token;
};

};