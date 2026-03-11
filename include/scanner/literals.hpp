#pragma once

#include <string>

#include "token_base.hpp"

namespace Scanner {

TOKEN(Identifier, std::string, "");

TOKEN(Integer, int, "");
TOKEN(Unsigned, unsigned, "");

using Literals = TTuple
< Identifier

, Integer
, Unsigned
>;

template<typename T>
concept literal = Contains<Literals, T>::value;

};