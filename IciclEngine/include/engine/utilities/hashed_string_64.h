#pragma once
#include <string>

struct hashed_string_64 {
    std::uint64_t hash;
    std::string string;

    hashed_string_64() : string(""), hash(fnv1a_64_hash("")) {}
    hashed_string_64(const char* s) : string(s), hash(fnv1a_64_hash(s)) {}

    operator std::uint64_t() const { return hash; }

private:
    static constexpr std::uint64_t fnv1a_64_hash(const char* s) {
        std::uint64_t hash = 14695981039346656037ull;
        while (*s)
        {
            hash = (hash ^ static_cast<std::uint64_t>(*s++)) * 1099511628211ull;
        }
        return hash;
    }
};