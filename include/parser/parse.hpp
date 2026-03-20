#pragma once

#include "scanner/token.hpp"
#include "parser/definition.hpp"

namespace Parser {
  Program parse(const std::vector<Scanner::TokenInfo> &tokens);
};