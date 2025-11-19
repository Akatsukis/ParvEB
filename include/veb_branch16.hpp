#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>

#include "veb_leaf8.hpp"

namespace veb_detail
{

    template <unsigned FanBits> class DenseBitset
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

} // namespace veb_detail

class VebBranch16
{
public:
    using Key = uint32_t;
    static constexpr unsigned SUBTREE_BITS = 16;
    static constexpr Key SUBTREE_SIZE = Key(1) << SUBTREE_BITS;
    static constexpr Key MAX_KEY = SUBTREE_SIZE - 1;
    static constexpr Key PREDECESSOR_QUERY_MAX = SUBTREE_SIZE;

private:
    static constexpr unsigned CLUSTER_BITS = 8;
    static constexpr unsigned CLUSTER_COUNT = 1u << CLUSTER_BITS;
    static constexpr Key CHILD_MASK = (Key(1) << CLUSTER_BITS) - 1;

    using Summary = VebLeaf8;
    using DenseMask = veb_detail::DenseBitset<CLUSTER_BITS>;

    DenseMask inline_mask_{};
    std::array<uint8_t, CLUSTER_COUNT> inline_value_{};
    std::array<std::unique_ptr<VebLeaf8>, CLUSTER_COUNT> clusters_{};
    std::unique_ptr<Summary> summary_{};

    [[nodiscard]] static Key combine(unsigned hi, uint16_t lo) noexcept
    {
        return (Key(hi) << CLUSTER_BITS) | Key(lo);
    }

    [[nodiscard]] Summary &ensure_summary()
    {
        if (!summary_) {
            summary_ = std::make_unique<Summary>();
        }
        return *summary_;
    }

    void summary_insert(unsigned idx)
    {
        ensure_summary().insert(static_cast<Summary::Key>(idx));
    }

    void summary_erase(unsigned idx)
    {
        if (!summary_) {
            return;
        }
        summary_->erase(static_cast<Summary::Key>(idx));
        if (summary_->empty()) {
            summary_.reset();
        }
    }

    [[nodiscard]] std::optional<unsigned>
    summary_successor(unsigned idx) const noexcept
    {
        if (!summary_) {
            return std::nullopt;
        }
        auto next = summary_->successor(static_cast<Summary::Key>(idx));
        if (!next) {
            return std::nullopt;
        }
        return static_cast<unsigned>(*next);
    }

    [[nodiscard]] std::optional<unsigned>
    summary_predecessor(unsigned idx) const noexcept
    {
        if (!summary_) {
            return std::nullopt;
        }
        auto prev = summary_->predecessor(static_cast<Summary::Key>(idx));
        if (!prev) {
            return std::nullopt;
        }
        return static_cast<unsigned>(*prev);
    }

    [[nodiscard]] bool cluster_active(unsigned idx) const noexcept
    {
        return inline_mask_.test(idx) || clusters_[idx] != nullptr;
    }

    [[nodiscard]] VebLeaf8 &ensure_cluster(unsigned idx)
    {
        auto &ptr = clusters_[idx];
        if (!ptr) {
            ptr = std::make_unique<VebLeaf8>();
        }
        return *ptr;
    }

public:
    bool empty() const noexcept
    {
        return !summary_;
    }

    void insert(Key key)
    {
        assert(key <= MAX_KEY);
        unsigned hi = static_cast<unsigned>(key >> CLUSTER_BITS);
        uint16_t lo = static_cast<uint16_t>(key & CHILD_MASK);

        if (!cluster_active(hi)) {
            summary_insert(hi);
            inline_mask_.set(hi);
            inline_value_[hi] = static_cast<uint8_t>(lo);
            return;
        }

        if (inline_mask_.test(hi)) {
            if (inline_value_[hi] == lo) {
                return;
            }
            VebLeaf8 &leaf = ensure_cluster(hi);
            leaf.insert(static_cast<uint16_t>(inline_value_[hi]));
            inline_mask_.reset(hi);
            leaf.insert(lo);
            return;
        }

        ensure_cluster(hi).insert(lo);
    }

    void erase(Key key) noexcept
    {
        if (key > MAX_KEY) {
            return;
        }
        unsigned hi = static_cast<unsigned>(key >> CLUSTER_BITS);
        uint16_t lo = static_cast<uint16_t>(key & CHILD_MASK);
        if (!cluster_active(hi)) {
            return;
        }
        if (inline_mask_.test(hi)) {
            if (inline_value_[hi] != static_cast<uint8_t>(lo)) {
                return;
            }
            inline_mask_.reset(hi);
            summary_erase(hi);
            return;
        }
        auto &ptr = clusters_[hi];
        if (!ptr) {
            return;
        }
        ptr->erase(lo);
        if (ptr->empty()) {
            ptr.reset();
            summary_erase(hi);
        }
    }

    [[nodiscard]] bool contains(Key key) const noexcept
    {
        if (key > MAX_KEY) {
            return false;
        }
        unsigned hi = static_cast<unsigned>(key >> CLUSTER_BITS);
        uint16_t lo = static_cast<uint16_t>(key & CHILD_MASK);
        if (inline_mask_.test(hi)) {
            return inline_value_[hi] == static_cast<uint8_t>(lo);
        }
        auto const &ptr = clusters_[hi];
        return ptr && ptr->contains(lo);
    }

    [[nodiscard]] std::optional<Key> min() const noexcept
    {
        if (!summary_) {
            return std::nullopt;
        }
        auto hi = summary_->min();
        if (!hi) {
            return std::nullopt;
        }
        unsigned idx = static_cast<unsigned>(*hi);
        if (inline_mask_.test(idx)) {
            return combine(idx, static_cast<uint16_t>(inline_value_[idx]));
        }
        auto const &ptr = clusters_[idx];
        if (!ptr) {
            return std::nullopt;
        }
        auto lo = ptr->min();
        if (!lo) {
            return std::nullopt;
        }
        return combine(idx, static_cast<uint16_t>(*lo));
    }

    [[nodiscard]] std::optional<Key> max() const noexcept
    {
        if (!summary_) {
            return std::nullopt;
        }
        auto hi = summary_->max();
        if (!hi) {
            return std::nullopt;
        }
        unsigned idx = static_cast<unsigned>(*hi);
        if (inline_mask_.test(idx)) {
            return combine(idx, static_cast<uint16_t>(inline_value_[idx]));
        }
        auto const &ptr = clusters_[idx];
        if (!ptr) {
            return std::nullopt;
        }
        auto lo = ptr->max();
        if (!lo) {
            return std::nullopt;
        }
        return combine(idx, static_cast<uint16_t>(*lo));
    }

    [[nodiscard]] std::optional<Key> successor(Key key) const noexcept
    {
        if (!summary_) {
            return std::nullopt;
        }
        if (key >= MAX_KEY) {
            auto hi = summary_->successor(static_cast<Summary::Key>(CLUSTER_COUNT - 1));
            if (!hi) {
                return std::nullopt;
            }
            unsigned idx = static_cast<unsigned>(*hi);
            if (inline_mask_.test(idx)) {
                return combine(idx, static_cast<uint16_t>(inline_value_[idx]));
            }
            auto lo = clusters_[idx]->min();
            return lo ? std::optional<Key>(combine(idx, static_cast<uint16_t>(*lo)))
                      : std::nullopt;
        }
        unsigned hi = static_cast<unsigned>(key >> CLUSTER_BITS);
        uint16_t lo = static_cast<uint16_t>(key & CHILD_MASK);
        if (cluster_active(hi)) {
            if (inline_mask_.test(hi)) {
                if (inline_value_[hi] > static_cast<uint8_t>(lo)) {
                    return combine(hi, static_cast<uint16_t>(inline_value_[hi]));
                }
            }
            else if (auto const &ptr = clusters_[hi]) {
                if (auto s = ptr->successor(lo)) {
                    return combine(hi, static_cast<uint16_t>(*s));
                }
            }
        }
        auto next = summary_successor(hi);
        if (!next) {
            return std::nullopt;
        }
        unsigned idx = *next;
        if (inline_mask_.test(idx)) {
            return combine(idx, static_cast<uint16_t>(inline_value_[idx]));
        }
        auto lo_min = clusters_[idx]->min();
        if (!lo_min) {
            return std::nullopt;
        }
        return combine(idx, static_cast<uint16_t>(*lo_min));
    }

    [[nodiscard]] std::optional<Key> predecessor(Key key) const noexcept
    {
        if (!summary_) {
            return std::nullopt;
        }
        if (key == 0) {
            return std::nullopt;
        }
        bool overflow = key > MAX_KEY;
        unsigned hi;
        uint16_t limit;
        if (overflow) {
            hi = CLUSTER_COUNT - 1;
            limit = Summary::PREDECESSOR_QUERY_MAX;
        }
        else {
            hi = static_cast<unsigned>(key >> CLUSTER_BITS);
            limit = static_cast<uint16_t>(key & CHILD_MASK);
        }

        if (cluster_active(hi)) {
            if (inline_mask_.test(hi)) {
                if (!overflow &&
                    inline_value_[hi] < static_cast<uint8_t>(limit)) {
                    return combine(hi, static_cast<uint16_t>(inline_value_[hi]));
                }
                if (overflow) {
                    return combine(hi, static_cast<uint16_t>(inline_value_[hi]));
                }
            }
            else if (auto const &ptr = clusters_[hi]) {
                uint16_t child_limit =
                    overflow ? Summary::PREDECESSOR_QUERY_MAX : limit;
                if (auto p = ptr->predecessor(child_limit)) {
                    return combine(hi, static_cast<uint16_t>(*p));
                }
            }
        }

        auto prev =
            summary_predecessor(overflow ? hi + 1 : hi);
        if (!prev) {
            return std::nullopt;
        }
        unsigned idx = *prev;
        if (inline_mask_.test(idx)) {
            return combine(idx, static_cast<uint16_t>(inline_value_[idx]));
        }
        auto lo_max = clusters_[idx]->max();
        if (!lo_max) {
            return std::nullopt;
        }
        return combine(idx, static_cast<uint16_t>(*lo_max));
    }

    template <class Fn> void for_each(Key prefix, Fn &&fn) const
    {
        if (!summary_) {
            return;
        }
        summary_->for_each([&](Summary::Key cluster_idx) {
            unsigned hi = static_cast<unsigned>(cluster_idx);
            Key child_prefix = prefix | (Key(hi) << CLUSTER_BITS);
            if (inline_mask_.test(hi)) {
                fn(child_prefix |
                   static_cast<uint16_t>(inline_value_[hi]));
            }
            else if (clusters_[hi]) {
                clusters_[hi]->for_each(child_prefix, fn);
            }
        });
    }

    template <class Fn> void for_each(Fn &&fn) const
    {
        for_each(0, std::forward<Fn>(fn));
    }
};
