#pragma once

#include "scanner/common/token.hpp"
#include "parser/common/definition.hpp"

namespace Parser {

using TokenIterator = std::vector<Scanner::TokenInfo>::const_iterator;
Program parse(const std::vector<Scanner::TokenInfo> &tokens);

}