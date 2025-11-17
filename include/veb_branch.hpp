#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

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
    uint64_t inline_mask_{0};
    std::array<Key, FANOUT> inline_value_{};
    std::array<std::unique_ptr<Child>, FANOUT> child{};

    Child const *get_child(unsigned i) const noexcept
    {
        if (i >= FANOUT) {
            return nullptr;
        }
        if (!(occ & (uint64_t(1) << i))) {
            return nullptr;
        }
        if (inline_mask_ & (uint64_t(1) << i)) {
            return nullptr;
        }
        return child[i].get();
    }

    Child *get_child(unsigned i) noexcept
    {
        if (i >= FANOUT) {
            return nullptr;
        }
        if (!(occ & (uint64_t(1) << i))) {
            return nullptr;
        }
        if (inline_mask_ & (uint64_t(1) << i)) {
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

    void insert(Key key)
    {
        unsigned hi = key >> CHILD_BITS;
        Key lo = key & CHILD_MASK;
        uint64_t bit = uint64_t(1) << hi;
        if (!(occ & bit)) {
            occ |= bit;
            inline_mask_ |= bit;
            inline_value_[hi] = lo;
            return;
        }

        if (inline_mask_ & bit) {
            Key existing = inline_value_[hi];
            if (existing == lo) {
                return;
            }
            inline_mask_ &= ~bit;
            auto &node = child[hi];
            node = std::make_unique<Child>();
            node->insert(existing);
            node->insert(lo);
            return;
        }

        auto &node = child[hi];
        if (!node) {
            node = std::make_unique<Child>();
        }
        node->insert(lo);
    }

    void erase(Key key) noexcept
    {
        unsigned hi = key >> CHILD_BITS;
        Key lo = key & CHILD_MASK;
        uint64_t bit = uint64_t(1) << hi;
        if (!(occ & bit)) {
            return;
        }
        if (inline_mask_ & bit) {
            if (inline_value_[hi] == lo) {
                inline_mask_ &= ~bit;
                occ &= ~bit;
            }
            return;
        }
        auto &node = child[hi];
        if (!node) {
            return;
        }
        node->erase(lo);
        if (node->empty()) {
            occ &= ~bit;
            node.reset();
        }
    }

    bool contains(Key key) const noexcept
    {
        unsigned hi = key >> CHILD_BITS;
        Key lo = key & CHILD_MASK;
        uint64_t bit = uint64_t(1) << hi;
        if (!(occ & bit)) {
            return false;
        }
        if (inline_mask_ & bit) {
            return inline_value_[hi] == lo;
        }
        Child const *c = child[hi].get();
        return c && c->contains(lo);
    }

    std::optional<Key> min() const noexcept
    {
        if (!occ) {
            return std::nullopt;
        }
        int hi = first_set(occ);
        uint64_t bit = uint64_t(1) << hi;
        if (inline_mask_ & bit) {
            return (Key(hi) << CHILD_BITS) | inline_value_[hi];
        }
        Child const *c = child[hi].get();
        auto lo = c ? c->min() : std::nullopt;
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
        uint64_t bit = uint64_t(1) << hi;
        if (inline_mask_ & bit) {
            return (Key(hi) << CHILD_BITS) | inline_value_[hi];
        }
        Child const *c = child[hi].get();
        auto lo = c ? c->max() : std::nullopt;
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
        uint64_t bit = uint64_t(1) << hi;
        if (occ & bit) {
            if (inline_mask_ & bit) {
                Key stored = inline_value_[hi];
                if (stored > lo) {
                    return (Key(hi) << CHILD_BITS) | stored;
                }
            }
            else if (Child const *c = child[hi].get()) {
                if (auto s_lo = c->successor(lo)) {
                    return (Key(hi) << CHILD_BITS) | *s_lo;
                }
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
        uint64_t bit2 = uint64_t(1) << hi2;
        if (inline_mask_ & bit2) {
            return (Key(hi2) << CHILD_BITS) | inline_value_[hi2];
        }
        Child const *c2 = child[hi2].get();
        auto lo_min = c2 ? c2->min() : std::nullopt;
        if (!lo_min) {
            return std::nullopt;
        }
        return (Key(hi2) << CHILD_BITS) | *lo_min;
    }

    std::optional<Key> predecessor(Key key) const noexcept
    {
        if (!occ) {
            return std::nullopt;
        }
        unsigned hi = key >> CHILD_BITS;
        Key lo = key & CHILD_MASK;

        uint64_t bit = uint64_t(1) << hi;
        if (occ & bit) {
            if (inline_mask_ & bit) {
                Key stored = inline_value_[hi];
                if (stored < lo) {
                    return (Key(hi) << CHILD_BITS) | stored;
                }
            }
            else if (Child const *c = child[hi].get()) {
                if (auto p_lo = c->predecessor(lo)) {
                    return (Key(hi) << CHILD_BITS) | *p_lo;
                }
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
        uint64_t bit2 = uint64_t(1) << hi2;
        if (inline_mask_ & bit2) {
            return (Key(hi2) << CHILD_BITS) | inline_value_[hi2];
        }
        Child const *c2 = child[hi2].get();
        auto lo_max = c2 ? c2->max() : std::nullopt;
        if (!lo_max) {
            return std::nullopt;
        }
        return (Key(hi2) << CHILD_BITS) | *lo_max;
    }

    template <class Fn>
    void for_each(Key prefix, Fn &&fn) const
    {
        uint64_t mask = occ;
        while (mask) {
            unsigned hi = std::countr_zero(mask);
            mask &= (mask - 1);
            Key child_prefix = prefix | (Key(hi) << CHILD_BITS);
            uint64_t bit = uint64_t(1) << hi;
            if (inline_mask_ & bit) {
                fn(child_prefix | inline_value_[hi]);
            }
            else if (child[hi]) {
                child[hi]->for_each(child_prefix, fn);
            }
        }
    }
};
