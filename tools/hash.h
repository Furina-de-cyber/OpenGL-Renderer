#pragma once

#include <memory>
#include <functional>
#include <algorithm>

#pragma once
#include <string>
#include <string_view>
#include <functional>
#include <type_traits>

namespace hash_utils
{
    inline void hash_combine(size_t& seed, size_t h)
    {
        seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    // 单个字符串的 hash（统一转成 string_view）
    inline size_t hash_string(std::string_view sv)
    {
        return std::hash<std::string_view>{}(sv);
    }

    template<typename... Strs>
    size_t hash_strings(const Strs&... strs)
    {
        static_assert(
            (std::is_convertible_v<Strs, std::string_view> && ...),
            "hash_strings only accepts string-like arguments"
            );

        size_t seed = 0;

        (
            hash_combine(seed, hash_string(std::string_view{ strs })),
            ...
            );

        return seed;
    }
}
