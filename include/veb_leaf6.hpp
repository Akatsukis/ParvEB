#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <span>

#include "simd_utils.hpp"

class VebLeaf6
{
public:
    using Key = uint8_t;
    static constexpr unsigned SUBTREE_BITS = 6;
    static constexpr Key SUBTREE_SIZE = Key(1) << SUBTREE_BITS;
    static constexpr Key MAX_KEY = SUBTREE_SIZE - 1;

private:
    uint64_t bits{0};

public:
    inline void insert(Key x) noexcept
    {
        bits |= (uint64_t(1) << x);
    }

    inline void erase(Key x) noexcept
    {
        bits &= ~(uint64_t(1) << x);
    }

    inline bool contains(Key x) const noexcept
    {
        return bits >> x & 1;
    }

    inline bool empty() const noexcept
    {
        return bits == 0;
    }

    [[nodiscard]] inline std::optional<Key> min() const noexcept
    {
        if (!bits) {
            return std::nullopt;
        }
        return static_cast<Key>(std::countr_zero(bits));
    }

    [[nodiscard]] inline std::optional<Key> max() const noexcept
    {
        if (!bits) {
            return std::nullopt;
        }
        return static_cast<Key>(63 - std::countl_zero(bits));
    }

    [[nodiscard]] inline std::optional<Key> successor(Key x) const noexcept
    {
        if (x >= 63) {
            return std::nullopt;
        }
        uint64_t mask = bits & (~0ull << (x + 1));
        if (!mask) {
            return std::nullopt;
        }
        return static_cast<Key>(std::countr_zero(mask));
    }

    [[nodiscard]] inline std::optional<Key> predecessor(Key x) const noexcept
    {
        if (x == 0) {
            return std::nullopt;
        }
        uint64_t mask;
        if (x == 64) {
            mask = bits;
        }
        else {
            mask = bits & ((uint64_t(1) << x) - 1);
        }
        if (!mask) {
            return std::nullopt;
        }
        return static_cast<Key>(63 - std::countl_zero(mask));
    }

    template <class Fn>
    void for_each(Key prefix, Fn &&fn) const
    {
        uint64_t x = bits;
        while (x) {
            unsigned bit = std::countr_zero(x);
            fn(prefix | Key(bit));
            x &= (x - 1);
        }
    }

    template <class Fn>
    void for_each(Fn &&fn) const
    {
        uint64_t x = bits;
        while (x) {
            unsigned bit = std::countr_zero(x);
            fn(Key(bit));
            x &= (x - 1);
        }
    }
};

static_assert(sizeof(VebLeaf6) == 8, "VebLeaf6 must be 8 bytes");
static_assert(alignof(VebLeaf6) == 8, "VebLeaf6 must be 8-byte aligned");
