#pragma once

#include <vector>
#include <fstream>

#include "token.hpp"

namespace Scanner {

std::vector<TokenInfo> scan(std::ifstream &source);

};