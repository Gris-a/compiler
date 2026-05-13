#pragma once

#include <functional>

inline void hash_combine(std::size_t &seed, std::size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}