#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <optional>
#include <span>

#include "simd_utils.hpp"

class VebLeaf8
{
public:
    using Key = uint32_t;
    static constexpr unsigned SUBTREE_BITS = 8;
    static constexpr Key SUBTREE_SIZE = Key(1) << SUBTREE_BITS;
    static constexpr Key MAX_KEY = SUBTREE_SIZE - 1;
    static constexpr Key PREDECESSOR_QUERY_MAX = MAX_KEY;

private:
    static constexpr unsigned WORD_BITS = 64;
    static constexpr unsigned WORD_COUNT = SUBTREE_SIZE / WORD_BITS;

    std::array<uint64_t, WORD_COUNT> words_{};

    static constexpr unsigned word_index(Key x) noexcept
    {
        return static_cast<unsigned>(x >> 6);
    }

    static constexpr uint64_t bit_mask(Key x) noexcept
    {
        return uint64_t(1) << (static_cast<unsigned>(x) & 63);
    }

    [[nodiscard]] static std::optional<Key>
    find_next(uint64_t word, unsigned base) noexcept
    {
        if (!word) {
            return std::nullopt;
        }
        return Key(base + std::countr_zero(word));
    }

    [[nodiscard]] static std::optional<Key>
    find_prev(uint64_t word, unsigned base) noexcept
    {
        if (!word) {
            return std::nullopt;
        }
        return Key(base + (63u - std::countl_zero(word)));
    }

    [[nodiscard]] std::optional<unsigned>
    next_nonzero_word(unsigned start_word) const noexcept
    {
        auto idx = simd::find_next_nonzero(
            std::span<uint64_t const>(words_), start_word);
        if (!idx) {
            return std::nullopt;
        }
        return static_cast<unsigned>(*idx);
    }

    [[nodiscard]] std::optional<unsigned>
    prev_nonzero_word(int start_word) const noexcept
    {
        if (start_word < 0) {
            return std::nullopt;
        }
        auto idx = simd::find_prev_nonzero(
            std::span<uint64_t const>(words_),
            static_cast<std::size_t>(start_word));
        if (!idx) {
            return std::nullopt;
        }
        return static_cast<unsigned>(*idx);
    }

public:
    void insert(Key x) noexcept
    {
        assert(x < SUBTREE_SIZE);
        words_[word_index(x)] |= bit_mask(x);
    }

    void erase(Key x) noexcept
    {
        assert(x < SUBTREE_SIZE);
        words_[word_index(x)] &= ~bit_mask(x);
    }

    [[nodiscard]] bool contains(Key x) const noexcept
    {
        assert(x < SUBTREE_SIZE);
        return (words_[word_index(x)] & bit_mask(x)) != 0;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        for (uint64_t word : words_) {
            if (word) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::optional<Key> min() const noexcept
    {
        auto word_idx = next_nonzero_word(0);
        if (!word_idx) {
            return std::nullopt;
        }
        return find_next(words_[*word_idx], *word_idx * WORD_BITS);
    }

    [[nodiscard]] std::optional<Key> max() const noexcept
    {
        auto word_idx = prev_nonzero_word(static_cast<int>(WORD_COUNT) - 1);
        if (!word_idx) {
            return std::nullopt;
        }
        return find_prev(words_[*word_idx], *word_idx * WORD_BITS);
    }

    [[nodiscard]] std::optional<Key> successor(Key x) const noexcept
    {
        assert(x < SUBTREE_SIZE);
        if (x >= MAX_KEY) {
            return std::nullopt;
        }
        unsigned idx = word_index(x);
        unsigned bit = static_cast<unsigned>(x) & 63;
        uint64_t mask = bit == 63 ? 0 : (~uint64_t(0) << (bit + 1));
        uint64_t word = words_[idx] & mask;
        if (auto next = find_next(word, idx * WORD_BITS)) {
            return next;
        }
        if (idx + 1 >= WORD_COUNT) {
            return std::nullopt;
        }
        auto next_word = next_nonzero_word(idx + 1);
        if (!next_word) {
            return std::nullopt;
        }
        return find_next(words_[*next_word], *next_word * WORD_BITS);
    }

    [[nodiscard]] std::optional<Key> predecessor(Key x) const noexcept
    {
        assert(x <= PREDECESSOR_QUERY_MAX);
        if (x == 0) {
            return std::nullopt;
        }
        Key search = x - 1;
        unsigned idx = word_index(search);
        unsigned bit = static_cast<unsigned>(search) & 63;
        uint64_t mask =
            bit == 63 ? ~uint64_t(0) : ((uint64_t(1) << (bit + 1)) - 1);
        uint64_t word = words_[idx] & mask;
        if (auto prev = find_prev(word, idx * WORD_BITS)) {
            return prev;
        }
        auto prev_word = prev_nonzero_word(static_cast<int>(idx) - 1);
        if (!prev_word) {
            return std::nullopt;
        }
        return find_prev(words_[*prev_word], *prev_word * WORD_BITS);
    }

    template <class Fn>
    void for_each(Key prefix, Fn &&fn) const
    {
        for (unsigned i = 0; i < WORD_COUNT; ++i) {
            uint64_t word = words_[i];
            while (word) {
                unsigned bit = std::countr_zero(word);
                fn(prefix | Key(i * WORD_BITS + bit));
                word &= (word - 1);
            }
        }
    }

    template <class Fn>
    void for_each(Fn &&fn) const
    {
        for_each(0, std::forward<Fn>(fn));
    }
};
