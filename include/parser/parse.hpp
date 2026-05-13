#pragma once

#include "scanner/token.hpp"
#include "parser/definition.hpp"

namespace Parser {

using TokenIterator = std::vector<Scanner::TokenInfo>::const_iterator;
Program parse(const std::vector<Scanner::TokenInfo> &tokens);

}