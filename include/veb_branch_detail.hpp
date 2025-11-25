#pragma once

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "veb_leaf6.hpp"
#include "veb_leaf8.hpp"

template <unsigned Bits, bool Sparse>
class VebBranch;

namespace veb_detail
{

    template <unsigned FanBits, bool Small = (FanBits <= 6)>
    class DenseBitset;

    // Fast path for fanout <= 64 (single 64-bit mask).
    template <unsigned FanBits>
    class DenseBitset<FanBits, true>
    {
        static_assert(FanBits > 0 && FanBits <= 6);
        static constexpr unsigned FANOUT = 1u << FanBits;
        using word_t = typename decltype([] {
            if constexpr (FanBits <= 3) {
                return std::type_identity<uint8_t>{};
            }
            else if constexpr (FanBits <= 4) {
                return std::type_identity<uint16_t>{};
            }
            else if constexpr (FanBits <= 5) {
                return std::type_identity<uint32_t>{};
            }
            else {
                return std::type_identity<uint64_t>{};
            }
        }())::type;

    public:
        [[nodiscard]] bool test(unsigned idx) const noexcept
        {
            return idx < FANOUT && (bits_ & (word_t{1} << idx));
        }

        void set(unsigned idx) noexcept
        {
            bits_ |= (word_t{1} << idx);
        }

        void reset(unsigned idx) noexcept
        {
            bits_ &= ~(word_t{1} << idx);
        }

    private:
        word_t bits_{0};
    };

    // General case: bitset over an array of 64-bit words.
    template <unsigned FanBits>
    class DenseBitset<FanBits, false>
    {
        static_assert(FanBits > 0);
        static constexpr unsigned WORD_BITS = 64;
        static constexpr unsigned FANOUT = 1u << FanBits;
        static constexpr unsigned WORD_COUNT =
            (FANOUT + WORD_BITS - 1) / WORD_BITS;

    public:
        [[nodiscard]] bool test(unsigned idx) const noexcept
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
    private:
        template <unsigned B>
        static consteval auto select_key()
        {
            static_assert(
                B <= 64, "key type selection only supports up to 64 bits");
            if constexpr (B <= 8) {
                return std::type_identity<uint8_t>{};
            }
            else if constexpr (B <= 16) {
                return std::type_identity<uint16_t>{};
            }
            else if constexpr (B <= 32) {
                return std::type_identity<uint32_t>{};
            }
            else {
                return std::type_identity<uint64_t>{};
            }
        }

    public:
        using type = typename decltype(select_key<Bits>())::type;
    };

    template <unsigned Bits, bool Sparse>
    struct ChildSelector
    {
        static constexpr unsigned CHILD_BITS = Bits / 2;
        static constexpr bool CHILD_SPARSE =
            default_sparse_storage<CHILD_BITS>();

        using type = VebBranch<CHILD_BITS, CHILD_SPARSE>;
    };

    template <bool Sparse>
    struct ChildSelector<12, Sparse>
    {
        using type = VebLeaf6;
    };

    template <bool Sparse>
    struct ChildSelector<16, Sparse>
    {
        using type = VebLeaf8;
    };

} // namespace veb_detail

template <
    unsigned Bits, bool Sparse = veb_detail::default_sparse_storage<Bits>()>
class VebBranch;
