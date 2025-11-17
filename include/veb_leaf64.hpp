#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>
#include <optional>
#include <utility>

class Leaf64
{
public:
    static constexpr unsigned MAX_VAL = 64;
    static constexpr unsigned SUBTREE_BITS = 6;

private:
    uint64_t bits{0};

public:
    inline void insert(unsigned x) noexcept
    {
        assert(x < MAX_VAL);
        bits |= (uint64_t(1) << x);
    }

    inline void erase(unsigned x) noexcept
    {
        assert(x < MAX_VAL);
        bits &= ~(uint64_t(1) << x);
    }

    inline bool contains(unsigned x) const noexcept
    {
        assert(x < MAX_VAL);
        return ((bits >> x) & 1ull) != 0;
    }

    inline bool empty() const noexcept
    {
        return bits == 0;
    }

    inline std::optional<unsigned> min() const noexcept
    {
        if (!bits) {
            return std::nullopt;
        }
        return static_cast<unsigned>(std::countr_zero(bits));
    }

    inline std::optional<unsigned> max() const noexcept
    {
        if (!bits) {
            return std::nullopt;
        }
        return static_cast<unsigned>(63 - std::countl_zero(bits));
    }

    inline std::optional<unsigned> successor(unsigned x) const noexcept
    {
        assert(x < MAX_VAL);
        if (x >= 63) {
            return std::nullopt;
        }
        uint64_t mask = bits & (~0ull << (x + 1));
        if (!mask) {
            return std::nullopt;
        }
        return static_cast<unsigned>(std::countr_zero(mask));
    }

    inline std::optional<unsigned> predecessor(unsigned x) const noexcept
    {
        assert(x <= MAX_VAL);
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
        return static_cast<unsigned>(63 - std::countl_zero(mask));
    }

    template <class Fn>
    inline void for_each(uint64_t prefix, Fn &&fn) const
    {
        uint64_t remaining = bits;
        while (remaining) {
            unsigned bit = std::countr_zero(remaining);
            fn(prefix | bit);
            remaining &= (remaining - 1);
        }
    }
};
