#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <utility>

class VebLeaf8
{
public:
    using Key = uint16_t;
    static constexpr unsigned SUBTREE_BITS = 8;
    static constexpr Key SUBTREE_SIZE = Key(1) << SUBTREE_BITS;
    static constexpr Key MAX_KEY = SUBTREE_SIZE - 1;

private:
    static constexpr unsigned WORD_BITS = 64;
    static constexpr unsigned WORD_COUNT = 4;
    std::array<uint64_t, WORD_COUNT> words_{};

    [[nodiscard]] static constexpr std::pair<unsigned, uint64_t>
    locate(Key x) noexcept
    {
        return {static_cast<unsigned>(x >> 6), uint64_t(1) << (x & 63)};
    }

public:
    inline void insert(Key x) noexcept
    {
        auto [word_idx, mask] = locate(x);
        words_[word_idx] |= mask;
    }

    inline void erase(Key x) noexcept
    {
        auto [word_idx, mask] = locate(x);
        words_[word_idx] &= ~mask;
    }

    template <class It>
    inline void batch_insert(It first, It last) noexcept
    {
        uint64_t accum[WORD_COUNT]{};
        for (; first != last; ++first) {
            auto [idx, mask] = locate(*first);
            accum[idx] |= mask;
        }
        for (unsigned i = 0; i < WORD_COUNT; ++i) {
            words_[i] |= accum[i];
        }
    }

    template <class It>
    inline void batch_erase(It first, It last) noexcept
    {
        uint64_t accum[WORD_COUNT]{};
        for (; first != last; ++first) {
            auto [idx, mask] = locate(*first);
            accum[idx] |= mask;
        }
        for (unsigned i = 0; i < WORD_COUNT; ++i) {
            words_[i] &= ~accum[i];
        }
    }

    inline void batch_insert(std::span<Key const> keys) noexcept
    {
        batch_insert(keys.begin(), keys.end());
    }

    inline void batch_erase(std::span<Key const> keys) noexcept
    {
        batch_erase(keys.begin(), keys.end());
    }

    [[nodiscard]] inline bool contains(Key x) const noexcept
    {
        auto [word_idx, mask] = locate(x);
        return (words_[word_idx] & mask) != 0;
    }

    [[nodiscard]] inline bool empty() const noexcept
    {
        return (words_[0] | words_[1] | words_[2] | words_[3]) == 0;
    }

    [[nodiscard]] inline std::optional<Key> min() const noexcept
    {
        for (unsigned i = 0; i < WORD_COUNT; ++i) {
            uint64_t word = words_[i];
            if (word) {
                return static_cast<Key>(i * WORD_BITS + std::countr_zero(word));
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] inline std::optional<Key> max() const noexcept
    {
        for (int i = WORD_COUNT - 1; i >= 0; --i) {
            uint64_t word = words_[static_cast<std::size_t>(i)];
            if (word) {
                return static_cast<Key>(
                    i * WORD_BITS + (63 - std::countl_zero(word)));
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] inline std::optional<Key> successor(Key x) const noexcept
    {
        if (x == MAX_KEY) {
            return std::nullopt;
        }
        unsigned word_idx = static_cast<unsigned>(x >> 6);
        unsigned offset = static_cast<unsigned>(x & 63);

        uint64_t word = words_[word_idx];
        uint64_t mask = offset == 63 ? 0 : (~0ull << (offset + 1));
        uint64_t candidate = word & mask;
        if (candidate) {
            return static_cast<Key>(
                word_idx * WORD_BITS + std::countr_zero(candidate));
        }

        for (unsigned i = word_idx + 1; i < WORD_COUNT; ++i) {
            uint64_t next_word = words_[i];
            if (next_word) {
                return static_cast<Key>(
                    i * WORD_BITS + std::countr_zero(next_word));
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] inline std::optional<Key> predecessor(Key x) const noexcept
    {
        if (x == 0) {
            return std::nullopt;
        }
        unsigned word_idx = static_cast<unsigned>(x >> 6);
        unsigned offset = static_cast<unsigned>(x & 63);

        uint64_t word = words_[word_idx];
        uint64_t mask = offset == 0 ? 0 : ((uint64_t(1) << offset) - 1);
        uint64_t candidate = word & mask;
        if (candidate) {
            return static_cast<Key>(
                word_idx * WORD_BITS + (63 - std::countl_zero(candidate)));
        }

        while (word_idx-- > 0) {
            uint64_t prev_word = words_[word_idx];
            if (prev_word) {
                return static_cast<Key>(
                    word_idx * WORD_BITS + (63 - std::countl_zero(prev_word)));
            }
        }
        return std::nullopt;
    }

    template <class Fn>
    void for_each(Key prefix, Fn &&fn) const
    {
        for (unsigned i = 0; i < WORD_COUNT; ++i) {
            uint64_t word = words_[i];
            while (word) {
                unsigned bit = std::countr_zero(word);
                fn(prefix | static_cast<Key>(i * WORD_BITS + bit));
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

static_assert(sizeof(VebLeaf8) == 32, "VebLeaf8 must be 32 bytes");
static_assert(alignof(VebLeaf8) == 8, "VebLeaf8 must be 8-byte aligned");
