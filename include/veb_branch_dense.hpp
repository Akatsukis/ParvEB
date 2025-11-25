#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include "veb_branch_detail.hpp"

template <unsigned Bits, bool Sparse>
class VebBranch;

// Dense specialization (array-backed clusters).
template <unsigned Bits>
class VebBranch<Bits, false>
{
    static_assert(Bits >= 8 && Bits % 2 == 0);
    static constexpr unsigned CLUSTER_BITS = Bits / 2;
    using Child = typename veb_detail::ChildSelector<Bits, false>::type;
    using Summary = Child;
    using DenseMask = veb_detail::DenseBitset<CLUSTER_BITS>;
    using ChildPtr = std::unique_ptr<Child>;
    static constexpr bool INLINE_CHILDREN = (Bits <= 16);

public:
    using Key = typename veb_detail::key_type_for_bits<Bits>::type;
    static constexpr unsigned SUBTREE_BITS = Bits;
    static constexpr Key MAX_KEY = [] {
        if constexpr (Bits == 64) {
            return std::numeric_limits<Key>::max();
        }
        else {
            return static_cast<Key>((uint64_t{1} << Bits) - 1);
        }
    }();
    static constexpr Key MAX = MAX_KEY;
    static constexpr unsigned FANOUT_BITS = CLUSTER_BITS;

    VebBranch() = default;

    bool empty() const noexcept
    {
        return !summary_;
    }

    void insert(Key key)
    {
        unsigned hi = static_cast<unsigned>(key >> CLUSTER_BITS);
        ChildKey lo = static_cast<ChildKey>(key & CHILD_MASK);

        if (!cluster_active(hi)) {
            summary_insert(hi);
            inline_mask_.set(hi);
            inline_value_[hi] = lo;
            return;
        }

        if (inline_mask_.test(hi)) {
            if (inline_value_[hi] == lo) {
                return;
            }
            Child &child = ensure_cluster(hi);
            child.insert(inline_value_[hi]);
            inline_mask_.reset(hi);
            child.insert(lo);
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
        ChildKey lo = static_cast<ChildKey>(key & CHILD_MASK);
        if (!cluster_active(hi)) {
            return;
        }
        if (inline_mask_.test(hi)) {
            if (inline_value_[hi] != lo) {
                return;
            }
            inline_mask_.reset(hi);
            summary_erase(hi);
            return;
        }
        if (auto *ptr = cluster_ptr(hi)) {
            ptr->erase(lo);
            if (ptr->empty()) {
                if constexpr (!INLINE_CHILDREN) {
                    clusters_[hi].reset();
                }
                cluster_mask_.reset(hi);
                summary_erase(hi);
            }
        }
    }

    [[nodiscard]] bool contains(Key key) const noexcept
    {
        if (key > MAX_KEY) {
            return false;
        }
        unsigned hi = static_cast<unsigned>(key >> CLUSTER_BITS);
        ChildKey lo = static_cast<ChildKey>(key & CHILD_MASK);
        if (inline_mask_.test(hi)) {
            return inline_value_[hi] == lo;
        }
        if (auto const *ptr = cluster_ptr(hi)) {
            return ptr->contains(lo);
        }
        return false;
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
            return combine(idx, inline_value_[idx]);
        }
        auto const *ptr = cluster_ptr(idx);
        if (!ptr) {
            return std::nullopt;
        }
        auto lo = ptr->min();
        if (!lo) {
            return std::nullopt;
        }
        return combine(idx, static_cast<ChildKey>(*lo));
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
            return combine(idx, inline_value_[idx]);
        }
        auto const *ptr = cluster_ptr(idx);
        if (!ptr) {
            return std::nullopt;
        }
        auto lo = ptr->max();
        if (!lo) {
            return std::nullopt;
        }
        return combine(idx, static_cast<ChildKey>(*lo));
    }

    [[nodiscard]] std::optional<Key> successor(Key key) const noexcept
    {
        if (!summary_) {
            return std::nullopt;
        }
        if (key >= MAX_KEY) {
            return std::nullopt;
        }
        unsigned hi = static_cast<unsigned>(key >> CLUSTER_BITS);
        ChildKey lo = static_cast<ChildKey>(key & CHILD_MASK);
        if (cluster_active(hi)) {
            if (inline_mask_.test(hi)) {
                if (inline_value_[hi] > lo) {
                    return combine(hi, inline_value_[hi]);
                }
            }
            else if (auto const *ptr = cluster_ptr(hi)) {
                if (auto s = ptr->successor(lo)) {
                    return combine(hi, static_cast<ChildKey>(*s));
                }
            }
        }
        auto next = summary_successor(hi);
        if (!next) {
            return std::nullopt;
        }
        unsigned idx = *next;
        if (inline_mask_.test(idx)) {
            return combine(idx, inline_value_[idx]);
        }
        auto lo_min = cluster_ptr(idx)->min();
        if (!lo_min) {
            return std::nullopt;
        }
        return combine(idx, static_cast<ChildKey>(*lo_min));
    }

    [[nodiscard]] std::optional<Key> predecessor(Key key) const noexcept
    {
        if (!summary_) {
            return std::nullopt;
        }
        if (key == 0 || key > MAX_KEY) {
            return std::nullopt;
        }
        unsigned hi = static_cast<unsigned>(key >> CLUSTER_BITS);
        ChildKey limit = static_cast<ChildKey>(key & CHILD_MASK);

        if (cluster_active(hi)) {
            if (inline_mask_.test(hi)) {
                if (inline_value_[hi] < limit) {
                    return combine(hi, inline_value_[hi]);
                }
            }
            else if (auto const *ptr = cluster_ptr(hi)) {
                if (auto p = ptr->predecessor(limit)) {
                    return combine(hi, static_cast<ChildKey>(*p));
                }
            }
        }

        auto prev = summary_predecessor(hi);
        if (!prev) {
            return std::nullopt;
        }
        unsigned idx = *prev;
        if (inline_mask_.test(idx)) {
            return combine(idx, inline_value_[idx]);
        }
        auto lo_max = cluster_ptr(idx)->max();
        if (!lo_max) {
            return std::nullopt;
        }
        return combine(idx, static_cast<ChildKey>(*lo_max));
    }

    template <class Fn>
    void for_each(Key prefix, Fn &&fn) const
    {
        if (!summary_) {
            return;
        }
        summary_->for_each([&](typename Summary::Key cluster_idx) {
            unsigned hi = static_cast<unsigned>(cluster_idx);
            Key child_prefix = prefix | (Key(hi) << CLUSTER_BITS);
            if (inline_mask_.test(hi)) {
                fn(child_prefix | inline_value_[hi]);
            }
            else if (auto const *ptr = cluster_ptr(hi)) {
                ptr->for_each(child_prefix, fn);
            }
        });
    }

    template <class Fn>
    void for_each(Fn &&fn) const
    {
        for_each(0, std::forward<Fn>(fn));
    }

private:
    using ChildKey = typename Child::Key;
    static constexpr std::size_t CLUSTER_COUNT =
        static_cast<std::size_t>(uint64_t{1} << CLUSTER_BITS);
    static constexpr Key CHILD_MASK = (Key(1) << CLUSTER_BITS) - 1;

    [[nodiscard]] static Key combine(unsigned hi, ChildKey lo) noexcept
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
        ensure_summary().insert(static_cast<typename Summary::Key>(idx));
    }

    void summary_erase(unsigned idx)
    {
        if (!summary_) {
            return;
        }
        summary_->erase(static_cast<typename Summary::Key>(idx));
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
        auto next =
            summary_->successor(static_cast<typename Summary::Key>(idx));
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
        auto prev =
            summary_->predecessor(static_cast<typename Summary::Key>(idx));
        if (!prev) {
            return std::nullopt;
        }
        return static_cast<unsigned>(*prev);
    }

    [[nodiscard]] bool cluster_active(unsigned idx) const noexcept
    {
        return inline_mask_.test(idx) || cluster_mask_.test(idx);
    }

    [[nodiscard]] Child &ensure_cluster(unsigned idx)
    {
        if constexpr (INLINE_CHILDREN) {
            cluster_mask_.set(idx);
            return clusters_[idx];
        }
        else {
            auto &ptr = clusters_[idx];
            if (!ptr) {
                ptr = std::make_unique<Child>();
                cluster_mask_.set(idx);
            }
            return *ptr;
        }
    }

    [[nodiscard]] Child *cluster_ptr(unsigned idx) noexcept
    {
        if constexpr (INLINE_CHILDREN) {
            return cluster_mask_.test(idx) ? &clusters_[idx] : nullptr;
        }
        else {
            return clusters_[idx].get();
        }
    }

    [[nodiscard]] Child const *cluster_ptr(unsigned idx) const noexcept
    {
        if constexpr (INLINE_CHILDREN) {
            return cluster_mask_.test(idx) ? &clusters_[idx] : nullptr;
        }
        else {
            return clusters_[idx].get();
        }
    }

    DenseMask inline_mask_{};
    DenseMask cluster_mask_{};
    std::array<ChildKey, CLUSTER_COUNT> inline_value_{};
    std::conditional_t<
        INLINE_CHILDREN, std::array<Child, CLUSTER_COUNT>,
        std::array<ChildPtr, CLUSTER_COUNT>>
        clusters_{};
    std::unique_ptr<Summary> summary_{};
};
