#pragma once

#include <vector>
#include <fstream>

#include "scanner/common/token.hpp"

namespace Scanner {

std::vector<TokenInfo> scan(std::ifstream &source);

}