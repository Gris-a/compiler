#pragma once

#include "scanner/token.hpp"
#include "parser/definition.hpp"

namespace Parser {
  std::vector<Definition> parse(const std::vector<Scanner::TokenInfo> &tokens);
};