#pragma once

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "veb_leaf8.hpp"

template <unsigned Bits, bool Sparse>
class VebBranch;

namespace veb_detail
{

    template <unsigned FanBits>
    class DenseBitset
    {
        static_assert(FanBits > 0);
        static constexpr unsigned WORD_BITS = 64;
        static constexpr unsigned FANOUT = 1u << FanBits;
        static constexpr unsigned WORD_COUNT =
            (FANOUT + WORD_BITS - 1) / WORD_BITS;

    public:
        bool test(unsigned idx) const noexcept
        {
            if (idx >= FANOUT) {
                return false;
            }
            auto [word_idx, mask] = locate(idx);
            return (words_[word_idx] & mask) != 0;
        }

        void set(unsigned idx) noexcept
        {
            auto [word_idx, mask] = locate(idx);
            words_[word_idx] |= mask;
        }

        void reset(unsigned idx) noexcept
        {
            auto [word_idx, mask] = locate(idx);
            words_[word_idx] &= ~mask;
        }

    private:
        static constexpr std::pair<unsigned, uint64_t>
        locate(unsigned idx) noexcept
        {
            return {idx >> 6, uint64_t(1) << (idx & 63)};
        }

        std::array<uint64_t, WORD_COUNT> words_{};
    };

    template <unsigned Bits>
    constexpr bool default_sparse_storage() noexcept
    {
        return Bits > 16;
    }

    template <unsigned Bits>
    struct key_type_for_bits
    {
        using type = std::conditional_t<(Bits <= 32), uint32_t, uint64_t>;
    };

    template <unsigned Bits, bool Sparse>
    struct ChildSelector
    {
        static constexpr unsigned CHILD_BITS = Bits / 2;
        static constexpr bool CHILD_SPARSE =
            default_sparse_storage<CHILD_BITS>();

        using type = std::conditional_t<
            CHILD_BITS == 8, VebLeaf8, VebBranch<CHILD_BITS, CHILD_SPARSE>>;
    };

} // namespace veb_detail

template <
    unsigned Bits, bool Sparse = veb_detail::default_sparse_storage<Bits>()>
class VebBranch;
