#pragma once

#define SINGLETON(name)                 \
name(const name &) = delete;            \
name &operator=(const name &) = delete; \
                                        \
name(name &&) = delete;                 \
name &operator=(name &&) = delete;      \
                                        \
static const name &get_instance() {     \
    static name instance;               \
    return instance;                    \
}
