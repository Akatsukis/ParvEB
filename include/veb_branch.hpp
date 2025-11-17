#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>

#include "veb_leaf64.hpp"

template <class Child, unsigned CHILD_BITS>
class Branch64
{
    static_assert(CHILD_BITS > 0, "CHILD_BITS must be > 0");
    static_assert(CHILD_BITS <= 63, "CHILD_BITS must be <= 63");
    static constexpr unsigned FAN_BITS = 6;
    static constexpr unsigned FANOUT = 1u << FAN_BITS;
    using Key = uint64_t;

    static constexpr Key child_mask_value() noexcept
    {
        if constexpr (CHILD_BITS >= 64) {
            return ~Key(0);
        }
        else {
            return (Key(1) << CHILD_BITS) - 1;
        }
    }

    static constexpr Key CHILD_MASK = child_mask_value();

    uint64_t occ{0};
    std::array<std::unique_ptr<Child>, FANOUT> child{};

    Child &ensure_child(unsigned i)
    {
        assert(i < FANOUT);
        if (!child[i]) {
            child[i] = std::make_unique<Child>();
        }
        occ |= (uint64_t(1) << i);
        return *child[i];
    }

    Child const *get_child(unsigned i) const noexcept
    {
        if (i >= FANOUT) {
            return nullptr;
        }
        return child[i].get();
    }

    Child *get_child(unsigned i) noexcept
    {
        if (i >= FANOUT) {
            return nullptr;
        }
        return child[i].get();
    }

    static int first_set(uint64_t x) noexcept
    {
        if (!x) {
            return -1;
        }
        return std::countr_zero(x);
    }

    static int last_set(uint64_t x) noexcept
    {
        if (!x) {
            return -1;
        }
        return 63 - std::countl_zero(x);
    }

public:
    static constexpr unsigned SUBTREE_BITS = FAN_BITS + CHILD_BITS;

    bool empty() const noexcept
    {
        return occ == 0;
    }

    void insert(Key key) noexcept
    {
        unsigned hi = key >> CHILD_BITS;
        Key lo = key & CHILD_MASK;
        Child &c = ensure_child(hi);
        c.insert(lo);
    }

    void erase(Key key) noexcept
    {
        unsigned hi = key >> CHILD_BITS;
        Key lo = key & CHILD_MASK;
        Child *c = get_child(hi);
        if (!c) {
            return;
        }
        c->erase(lo);
        if (c->empty()) {
            occ &= ~(uint64_t(1) << hi);
            child[hi].reset();
        }
    }

    bool contains(Key key) const noexcept
    {
        unsigned hi = key >> CHILD_BITS;
        Key lo = key & CHILD_MASK;
        Child const *c = get_child(hi);
        if (!c) {
            return false;
        }
        return c->contains(lo);
    }

    std::optional<Key> min() const noexcept
    {
        if (!occ) {
            return std::nullopt;
        }
        int hi = first_set(occ);
        Child const *c = get_child(hi);
        auto lo = c->min();
        if (!lo) {
            return std::nullopt;
        }
        return (Key(hi) << CHILD_BITS) | *lo;
    }

    std::optional<Key> max() const noexcept
    {
        if (!occ) {
            return std::nullopt;
        }
        int hi = last_set(occ);
        Child const *c = get_child(hi);
        auto lo = c->max();
        if (!lo) {
            return std::nullopt;
        }
        return (Key(hi) << CHILD_BITS) | *lo;
    }

    std::optional<Key> successor(Key key) const noexcept
    {
        if (!occ) {
            return std::nullopt;
        }
        unsigned hi = key >> CHILD_BITS;
        Key lo = key & CHILD_MASK;
        Child const *c = get_child(hi);
        if (c) {
            if (auto s_lo = c->successor(lo)) {
                return (Key(hi) << CHILD_BITS) | *s_lo;
            }
        }
        if (hi + 1 >= FANOUT) {
            return std::nullopt;
        }
        uint64_t mask = (hi + 1 >= 64) ? 0 : occ & (~0ull << (hi + 1));
        int hi2 = first_set(mask);
        if (hi2 < 0) {
            return std::nullopt;
        }
        Child const *c2 = get_child(hi2);
        auto lo_min = c2->min();
        return (Key(hi2) << CHILD_BITS) | *lo_min;
    }

    std::optional<Key> predecessor(Key key) const noexcept
    {
        if (!occ) {
            return std::nullopt;
        }
        unsigned hi = key >> CHILD_BITS;
        Key lo = key & CHILD_MASK;

        Child const *c = get_child(hi);
        if (c) {
            if (auto p_lo = c->predecessor(lo)) {
                return (Key(hi) << CHILD_BITS) | *p_lo;
            }
        }
        uint64_t mask;
        if (hi == 0) {
            mask = 0;
        }
        else {
            mask = occ & ((uint64_t(1) << hi) - 1);
        }
        int hi2 = last_set(mask);
        if (hi2 < 0) {
            return std::nullopt;
        }
        Child const *c2 = get_child(hi2);
        auto lo_max = c2->max();
        return (Key(hi2) << CHILD_BITS) | *lo_max;
    }
};
