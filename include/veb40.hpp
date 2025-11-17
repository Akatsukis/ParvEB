#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>

#include "veb_branch.hpp"
#include "veb_leaf64.hpp"

template <class Child, unsigned CHILD_BITS, unsigned TOP_BITS = 16>
class WideTopNode
{
    static_assert(CHILD_BITS > 0 && CHILD_BITS < 64, "CHILD_BITS must be in (0,64)");
    static_assert(TOP_BITS > 0 && TOP_BITS <= 32, "TOP_BITS must be <= 32");
    static constexpr unsigned WORD_BITS = 6;
    static constexpr unsigned WORD_SIZE = 1u << WORD_BITS;

public:
    using Key = uint64_t;
    static constexpr unsigned LOW_BITS = CHILD_BITS;
    static constexpr unsigned TOP_SIZE = 1u << TOP_BITS;
    static constexpr unsigned SUBTREE_BITS = TOP_BITS + CHILD_BITS;

private:
    static constexpr unsigned WORD_COUNT = TOP_SIZE / WORD_SIZE;
    static constexpr unsigned BLOCK_WORD_BITS = 6;
    static constexpr unsigned BLOCK_SIZE = 1u << BLOCK_WORD_BITS;
    static constexpr unsigned BLOCK_COUNT = WORD_COUNT / BLOCK_SIZE;
    static_assert(WORD_COUNT * WORD_SIZE == TOP_SIZE, "TOP_SIZE must be multiple of 64");
    static_assert(BLOCK_COUNT * BLOCK_SIZE == WORD_COUNT, "WORD_COUNT must be multiple of 64");

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

    Child &ensure_child(unsigned i)
    {
        assert(i < TOP_SIZE);
        if (!child_[i]) {
            child_[i] = std::make_unique<Child>();
        }
        set_occ(i);
        return *child_[i];
    }

    Child const *get_child(unsigned i) const noexcept
    {
        if (i >= TOP_SIZE) {
            return nullptr;
        }
        return child_[i].get();
    }

    Child *get_child(unsigned i) noexcept
    {
        if (i >= TOP_SIZE) {
            return nullptr;
        }
        return child_[i].get();
    }

    void set_occ(unsigned idx) noexcept
    {
        unsigned word_index = idx >> WORD_BITS;
        unsigned bit_index = idx & (WORD_SIZE - 1);
        uint64_t prev = occ_words_[word_index];
        occ_words_[word_index] = prev | (uint64_t(1) << bit_index);
        if (!prev) {
            unsigned block_index = word_index >> BLOCK_WORD_BITS;
            unsigned block_bit = word_index & (BLOCK_SIZE - 1);
            uint64_t block_prev = occ_blocks_[block_index];
            occ_blocks_[block_index] = block_prev | (uint64_t(1) << block_bit);
            if (!block_prev) {
                occ_super_ |= (uint64_t(1) << block_index);
            }
        }
    }

    void clear_occ(unsigned idx) noexcept
    {
        unsigned word_index = idx >> WORD_BITS;
        unsigned bit_index = idx & (WORD_SIZE - 1);
        uint64_t prev = occ_words_[word_index];
        uint64_t next = prev & ~(uint64_t(1) << bit_index);
        occ_words_[word_index] = next;
        if (next == 0 && prev != 0) {
            unsigned block_index = word_index >> BLOCK_WORD_BITS;
            unsigned block_bit = word_index & (BLOCK_SIZE - 1);
            uint64_t block_prev = occ_blocks_[block_index];
            uint64_t block_next = block_prev & ~(uint64_t(1) << block_bit);
            occ_blocks_[block_index] = block_next;
            if (block_next == 0 && block_prev != 0) {
                occ_super_ &= ~(uint64_t(1) << block_index);
            }
        }
    }

    int first_child_index() const noexcept
    {
        if (!occ_super_) {
            return -1;
        }
        unsigned block_index = std::countr_zero(occ_super_);
        uint64_t word_mask = occ_blocks_[block_index];
        unsigned word_in_block = std::countr_zero(word_mask);
        unsigned word_index = (block_index << BLOCK_WORD_BITS) + word_in_block;
        uint64_t bits = occ_words_[word_index];
        unsigned bit_index = std::countr_zero(bits);
        return static_cast<int>((word_index << WORD_BITS) + bit_index);
    }

    int last_child_index() const noexcept
    {
        if (!occ_super_) {
            return -1;
        }
        unsigned block_index = 63u - std::countl_zero(occ_super_);
        uint64_t word_mask = occ_blocks_[block_index];
        unsigned word_in_block = 63u - std::countl_zero(word_mask);
        unsigned word_index = (block_index << BLOCK_WORD_BITS) + word_in_block;
        uint64_t bits = occ_words_[word_index];
        unsigned bit_index = 63u - std::countl_zero(bits);
        return static_cast<int>((word_index << WORD_BITS) + bit_index);
    }

    int next_child_index(unsigned idx) const noexcept
    {
        if (idx + 1 >= TOP_SIZE) {
            return -1;
        }
        unsigned word_index = idx >> WORD_BITS;
        unsigned bit_index = idx & (WORD_SIZE - 1);
        uint64_t mask = (bit_index >= 63) ? 0 : (~0ull << (bit_index + 1));
        uint64_t bits = occ_words_[word_index] & mask;
        if (bits) {
            unsigned next_bit = std::countr_zero(bits);
            return static_cast<int>((word_index << WORD_BITS) + next_bit);
        }
        unsigned block_index = word_index >> BLOCK_WORD_BITS;
        unsigned block_bit = word_index & (BLOCK_SIZE - 1);
        uint64_t block_mask_bits = (block_bit >= 63) ? 0 : (~0ull << (block_bit + 1));
        uint64_t block_mask = occ_blocks_[block_index] & block_mask_bits;
        if (block_mask) {
            unsigned next_word_in_block = std::countr_zero(block_mask);
            word_index = (block_index << BLOCK_WORD_BITS) + next_word_in_block;
            uint64_t word_bits = occ_words_[word_index];
            unsigned next_bit = std::countr_zero(word_bits);
            return static_cast<int>((word_index << WORD_BITS) + next_bit);
        }
        if (occ_super_ == 0) {
            return -1;
        }
        uint64_t super_mask_bits = (block_index >= 63) ? 0 : (~0ull << (block_index + 1));
        uint64_t super_mask = occ_super_ & super_mask_bits;
        if (!super_mask) {
            return -1;
        }
        block_index = std::countr_zero(super_mask);
        uint64_t word_mask = occ_blocks_[block_index];
        unsigned word_in_block = std::countr_zero(word_mask);
        word_index = (block_index << BLOCK_WORD_BITS) + word_in_block;
        uint64_t word_bits = occ_words_[word_index];
        unsigned next_bit = std::countr_zero(word_bits);
        return static_cast<int>((word_index << WORD_BITS) + next_bit);
    }

    int prev_child_index(unsigned idx) const noexcept
    {
        if (idx == 0) {
            return -1;
        }
        if (idx > TOP_SIZE) {
            idx = TOP_SIZE;
        }
        unsigned word_index = (idx - 1) >> WORD_BITS;
        unsigned bit_index = (idx - 1) & (WORD_SIZE - 1);
        uint64_t mask = (bit_index == 63) ? ~uint64_t(0) : ((uint64_t(1) << (bit_index + 1)) - 1);
        uint64_t bits = occ_words_[word_index] & mask;
        if (bits) {
            unsigned prev_bit = 63u - std::countl_zero(bits);
            return static_cast<int>((word_index << WORD_BITS) + prev_bit);
        }
        if (word_index == 0) {
            return -1;
        }
        unsigned block_index = word_index >> BLOCK_WORD_BITS;
        unsigned block_bit = word_index & (BLOCK_SIZE - 1);
        uint64_t block_mask = block_bit == 0 ? 0 : ((uint64_t(1) << block_bit) - 1);
        uint64_t block_bits = occ_blocks_[block_index] & block_mask;
        if (block_bits) {
            unsigned prev_word_in_block = 63u - std::countl_zero(block_bits);
            word_index = (block_index << BLOCK_WORD_BITS) + prev_word_in_block;
            uint64_t word_bits = occ_words_[word_index];
            unsigned prev_bit = 63u - std::countl_zero(word_bits);
            return static_cast<int>((word_index << WORD_BITS) + prev_bit);
        }
        if (block_index == 0) {
            return -1;
        }
        uint64_t super_mask = (uint64_t(1) << block_index) - 1;
        super_mask &= occ_super_;
        if (!super_mask) {
            return -1;
        }
        block_index = 63u - std::countl_zero(super_mask);
        uint64_t word_mask = occ_blocks_[block_index];
        unsigned word_in_block = 63u - std::countl_zero(word_mask);
        word_index = (block_index << BLOCK_WORD_BITS) + word_in_block;
        uint64_t word_bits = occ_words_[word_index];
        unsigned prev_bit = 63u - std::countl_zero(word_bits);
        return static_cast<int>((word_index << WORD_BITS) + prev_bit);
    }

public:
    bool empty() const noexcept
    {
        return occ_super_ == 0;
    }

    void insert(Key key) noexcept
    {
        unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
        assert(hi < TOP_SIZE);
        Key lo = key & CHILD_MASK;
        Child &c = ensure_child(hi);
        c.insert(lo);
    }

    void erase(Key key) noexcept
    {
        unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
        if (hi >= TOP_SIZE) {
            return;
        }
        Key lo = key & CHILD_MASK;
        Child *c = get_child(hi);
        if (!c) {
            return;
        }
        c->erase(lo);
        if (c->empty()) {
            clear_occ(hi);
            child_[hi].reset();
        }
    }

    bool contains(Key key) const noexcept
    {
        unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
        if (hi >= TOP_SIZE) {
            return false;
        }
        Key lo = key & CHILD_MASK;
        Child const *c = get_child(hi);
        if (!c) {
            return false;
        }
        return c->contains(lo);
    }

    std::optional<Key> min() const noexcept
    {
        int hi = first_child_index();
        if (hi < 0) {
            return std::nullopt;
        }
        Child const *c = get_child(static_cast<unsigned>(hi));
        auto lo = c->min();
        if (!lo) {
            return std::nullopt;
        }
        return (Key(hi) << CHILD_BITS) | *lo;
    }

    std::optional<Key> max() const noexcept
    {
        int hi = last_child_index();
        if (hi < 0) {
            return std::nullopt;
        }
        Child const *c = get_child(static_cast<unsigned>(hi));
        auto lo = c->max();
        if (!lo) {
            return std::nullopt;
        }
        return (Key(hi) << CHILD_BITS) | *lo;
    }

    std::optional<Key> successor(Key key) const noexcept
    {
        if (empty()) {
            return std::nullopt;
        }
        unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
        Key lo = key & CHILD_MASK;
        Child const *c = get_child(hi);
        if (c) {
            if (auto s_lo = c->successor(lo)) {
                return (Key(hi) << CHILD_BITS) | *s_lo;
            }
        }
        int hi2 = next_child_index(hi);
        if (hi2 < 0) {
            return std::nullopt;
        }
        Child const *c2 = get_child(static_cast<unsigned>(hi2));
        auto lo_min = c2->min();
        return (Key(hi2) << CHILD_BITS) | *lo_min;
    }

    std::optional<Key> predecessor(Key key) const noexcept
    {
        if (empty()) {
            return std::nullopt;
        }
        unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
        Key lo = key & CHILD_MASK;
        Child const *c = get_child(hi);
        if (c) {
            if (auto p_lo = c->predecessor(lo)) {
                return (Key(hi) << CHILD_BITS) | *p_lo;
            }
        }
        int hi2 = prev_child_index(hi);
        if (hi2 < 0) {
            return std::nullopt;
        }
        Child const *c2 = get_child(static_cast<unsigned>(hi2));
        auto lo_max = c2->max();
        return (Key(hi2) << CHILD_BITS) | *lo_max;
    }

private:
    std::array<uint64_t, WORD_COUNT> occ_words_{};
    std::array<uint64_t, BLOCK_COUNT> occ_blocks_{};
    uint64_t occ_super_{0};
    std::array<std::unique_ptr<Child>, TOP_SIZE> child_{};
};

template <class RootNode, unsigned EFFECTIVE_BITS>
class VanEmdeBoasTree
{
    static_assert(EFFECTIVE_BITS > 0, "EFFECTIVE_BITS must be > 0");
    static_assert(RootNode::SUBTREE_BITS >= EFFECTIVE_BITS,
                  "RootNode must cover at least EFFECTIVE_BITS bits");

public:
    using Key = uint64_t;
    static constexpr unsigned KEY_BITS = EFFECTIVE_BITS;

private:
    static constexpr Key compute_max_key() noexcept
    {
        if constexpr (EFFECTIVE_BITS >= 64) {
            return std::numeric_limits<Key>::max();
        }
        else {
            return (Key(1) << EFFECTIVE_BITS) - 1;
        }
    }

    static constexpr Key compute_predecessor_query_max() noexcept
    {
        if constexpr (EFFECTIVE_BITS >= 64) {
            return MAX_KEY;
        }
        else {
            return MAX_KEY + 1;
        }
    }

public:
    static constexpr Key MAX_KEY = compute_max_key();
    static constexpr Key PREDECESSOR_QUERY_MAX = compute_predecessor_query_max();

    bool empty() const noexcept
    {
        return root_.empty();
    }

    void insert(Key key) noexcept
    {
        assert(key <= MAX_KEY);
        root_.insert(key);
    }

    void erase(Key key) noexcept
    {
        assert(key <= MAX_KEY);
        root_.erase(key);
    }

    bool contains(Key key) const noexcept
    {
        assert(key <= MAX_KEY);
        return root_.contains(key);
    }

    std::optional<Key> min() const noexcept
    {
        return root_.min();
    }

    std::optional<Key> max() const noexcept
    {
        return root_.max();
    }

    std::optional<Key> successor(Key key) const noexcept
    {
        assert(key <= MAX_KEY);
        return root_.successor(key);
    }

    std::optional<Key> predecessor(Key key) const noexcept
    {
        assert(key <= PREDECESSOR_QUERY_MAX);
        return root_.predecessor(key);
    }

private:
    RootNode root_{};
};

using VebNode6 = Leaf64;
using VebNode12 = Branch64<VebNode6, VebNode6::SUBTREE_BITS>;
using VebNode18 = Branch64<VebNode12, VebNode12::SUBTREE_BITS>;
using VebNode24 = Branch64<VebNode18, VebNode18::SUBTREE_BITS>;
using VebTopNode = WideTopNode<VebNode24, VebNode24::SUBTREE_BITS>;

using VebTree40 = VanEmdeBoasTree<VebTopNode, 40>;
