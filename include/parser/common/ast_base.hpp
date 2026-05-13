#pragma once
#include "scanner/common/token.hpp"

namespace Parser {

struct ASTBase {
    Scanner::TokenInfo token_info;
};

} // namespace Parser