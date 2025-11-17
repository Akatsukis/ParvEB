#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "simd_utils.hpp"
#include "veb_types.hpp"

class VebTopNode
{
public:
    using Key = uint64_t;
    static constexpr unsigned TOP_BITS = 16;
    static constexpr unsigned TOP_SIZE = 1u << TOP_BITS;
    static constexpr unsigned WORD_BITS = 6;
    static constexpr unsigned WORD_SIZE = 1u << WORD_BITS;
    static constexpr unsigned WORD_COUNT = TOP_SIZE / WORD_SIZE;

    using Child = VebNode24;
    static constexpr unsigned CHILD_BITS = Child::SUBTREE_BITS;
    static constexpr unsigned SUBTREE_BITS = TOP_BITS + CHILD_BITS;
    static constexpr Key CHILD_MASK = (Key(1) << CHILD_BITS) - 1;

    VebTopNode() = default;

    bool empty() const noexcept;
    void insert(Key key);
    void erase(Key key) noexcept;
    bool contains(Key key) const noexcept;
    std::optional<Key> min() const noexcept;
    std::optional<Key> max() const noexcept;
    std::optional<Key> successor(Key key) const noexcept;
    std::optional<Key> predecessor(Key key) const noexcept;
    template <class Fn>
    void for_each(Fn &&fn) const
    {
        for (unsigned word_idx = 0; word_idx < WORD_COUNT; ++word_idx) {
            uint64_t bits = occ_words_[word_idx];
            while (bits) {
                unsigned bit = std::countr_zero(bits);
                bits &= (bits - 1);
                unsigned hi = (word_idx << WORD_BITS) + bit;
                Child const *c = get_child(hi);
                if (c) {
                    c->for_each(Key(hi) << CHILD_BITS, fn);
                }
            }
        }
    }

private:
    void set_occ_bit(unsigned idx) noexcept;
    void clear_occ_bit(unsigned idx) noexcept;

    Child *get_child(unsigned idx) noexcept;
    Child const *get_child(unsigned idx) const noexcept;
    Child &ensure_child(unsigned idx);

    std::optional<unsigned> find_next_word(unsigned start_word) const noexcept;
    std::optional<unsigned> find_prev_word(unsigned start_word) const noexcept;

    std::array<uint64_t, WORD_COUNT> occ_words_{};
    std::size_t child_count_{0};
    std::array<std::unique_ptr<Child>, TOP_SIZE> child_{};
};
