#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include <ankerl/unordered_dense.h>

#include "veb_branch_detail.hpp"

template <unsigned Bits, bool Sparse>
class VebBranch;

// Sparse specialization (hash-map-backed clusters).
template <unsigned Bits>
class VebBranch<Bits, true>
{
    static_assert(Bits >= 8 && Bits % 2 == 0);
    static constexpr unsigned CLUSTER_BITS = Bits / 2;
    using Child = typename veb_detail::ChildSelector<Bits, true>::type;
    using Summary = Child;
    using ChildKey = typename Child::Key;
    using ClusterKey = typename Summary::Key;

    struct ClusterEntry
    {
        bool inline_only = true;
        ChildKey inline_value = 0;
        std::unique_ptr<Child> child{};
    };

    using ClusterMap = ankerl::unordered_dense::map<ClusterKey, ClusterEntry>;

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
        return summary_.empty();
    }

    void insert(Key key)
    {
        ClusterKey hi = hi_part(key);
        ChildKey lo = static_cast<ChildKey>(key & CHILD_MASK);
        auto [it, inserted] = clusters_.try_emplace(hi);
        ClusterEntry &entry = it->second;
        if (inserted) {
            summary_.insert(hi);
            entry.inline_only = true;
            entry.inline_value = lo;
            return;
        }

        if (entry.inline_only) {
            if (entry.inline_value == lo) {
                return;
            }
            Child &child = ensure_child(entry);
            child.insert(entry.inline_value);
            child.insert(lo);
            entry.inline_only = false;
            return;
        }

        ensure_child(entry).insert(lo);
    }

    void erase(Key key) noexcept
    {
        ClusterKey hi = hi_part(key);
        ChildKey lo = static_cast<ChildKey>(key & CHILD_MASK);
        auto it = clusters_.find(hi);
        if (it == clusters_.end()) {
            return;
        }
        ClusterEntry &entry = it->second;
        if (entry.inline_only) {
            if (entry.inline_value != lo) {
                return;
            }
            remove_cluster(it);
            return;
        }
        if (!entry.child) {
            return;
        }
        entry.child->erase(lo);
        if (entry.child->empty()) {
            remove_cluster(it);
        }
    }

    [[nodiscard]] bool contains(Key key) const noexcept
    {
        ClusterKey hi = hi_part(key);
        ChildKey lo = static_cast<ChildKey>(key & CHILD_MASK);
        if (auto const *entry = find_cluster(hi)) {
            if (entry->inline_only) {
                return entry->inline_value == lo;
            }
            return entry->child && entry->child->contains(lo);
        }
        return false;
    }

    [[nodiscard]] std::optional<Key> min() const noexcept
    {
        auto hi = summary_.min();
        if (!hi) {
            return std::nullopt;
        }
        auto const *entry = find_cluster(*hi);
        if (!entry) {
            return std::nullopt;
        }
        if (entry->inline_only) {
            return combine(*hi, entry->inline_value);
        }
        if (!entry->child) {
            return std::nullopt;
        }
        auto lo = entry->child->min();
        if (!lo) {
            return std::nullopt;
        }
        return combine(*hi, static_cast<ChildKey>(*lo));
    }

    [[nodiscard]] std::optional<Key> max() const noexcept
    {
        auto hi = summary_.max();
        if (!hi) {
            return std::nullopt;
        }
        auto const *entry = find_cluster(*hi);
        if (!entry) {
            return std::nullopt;
        }
        if (entry->inline_only) {
            return combine(*hi, entry->inline_value);
        }
        if (!entry->child) {
            return std::nullopt;
        }
        auto lo = entry->child->max();
        if (!lo) {
            return std::nullopt;
        }
        return combine(*hi, static_cast<ChildKey>(*lo));
    }

    [[nodiscard]] std::optional<Key> successor(Key key) const noexcept
    {
        if (summary_.empty()) {
            return std::nullopt;
        }
        if (key >= MAX_KEY) {
            return std::nullopt;
        }
        ClusterKey hi = hi_part(key);
        ChildKey lo = static_cast<ChildKey>(key & CHILD_MASK);

        if (auto const *entry = find_cluster(hi)) {
            if (entry->inline_only) {
                if (entry->inline_value > lo) {
                    return combine(hi, entry->inline_value);
                }
            }
            else if (entry->child) {
                if (auto s = entry->child->successor(lo)) {
                    return combine(hi, static_cast<ChildKey>(*s));
                }
            }
        }

        auto next_hi = summary_.successor(hi);
        if (!next_hi) {
            return std::nullopt;
        }
        auto const *entry = find_cluster(*next_hi);
        if (!entry) {
            return std::nullopt;
        }
        if (entry->inline_only) {
            return combine(*next_hi, entry->inline_value);
        }
        if (!entry->child) {
            return std::nullopt;
        }
        auto lo_min = entry->child->min();
        if (!lo_min) {
            return std::nullopt;
        }
        return combine(*next_hi, static_cast<ChildKey>(*lo_min));
    }

    [[nodiscard]] std::optional<Key> predecessor(Key key) const noexcept
    {
        if (summary_.empty()) {
            return std::nullopt;
        }
        if (key == 0 || key > MAX_KEY) {
            return std::nullopt;
        }
        ClusterKey hi = hi_part(key);
        ChildKey limit = static_cast<ChildKey>(key & CHILD_MASK);

        if (auto const *entry = find_cluster(hi)) {
            if (entry->inline_only) {
                if (entry->inline_value < limit) {
                    return combine(hi, entry->inline_value);
                }
            }
            else if (entry->child) {
                if (auto p = entry->child->predecessor(limit)) {
                    return combine(hi, static_cast<ChildKey>(*p));
                }
            }
        }

        auto prev_hi = summary_.predecessor(hi);
        if (!prev_hi) {
            return std::nullopt;
        }
        auto const *entry = find_cluster(*prev_hi);
        if (!entry) {
            return std::nullopt;
        }
        if (entry->inline_only) {
            return combine(*prev_hi, entry->inline_value);
        }
        if (!entry->child) {
            return std::nullopt;
        }
        auto lo_max = entry->child->max();
        if (!lo_max) {
            return std::nullopt;
        }
        return combine(*prev_hi, static_cast<ChildKey>(*lo_max));
    }

    template <class Fn>
    void for_each(Key prefix, Fn &&fn) const
    {
        summary_.for_each([&](ClusterKey cluster_idx) {
            auto it = clusters_.find(cluster_idx);
            if (it == clusters_.end()) {
                return;
            }
            ClusterEntry const &entry = it->second;
            Key child_prefix = prefix | (Key(cluster_idx) << CLUSTER_BITS);
            if (entry.inline_only) {
                fn(child_prefix | entry.inline_value);
            }
            else if (entry.child) {
                entry.child->for_each(child_prefix, fn);
            }
        });
    }

    template <class Fn>
    void for_each(Fn &&fn) const
    {
        for_each(0, std::forward<Fn>(fn));
    }

private:
    static constexpr uint64_t CLUSTER_COUNT = uint64_t{1} << CLUSTER_BITS;
    static constexpr Key CHILD_MASK = (Key(1) << CLUSTER_BITS) - 1;

    [[nodiscard]] static ClusterKey hi_part(Key key) noexcept
    {
        return static_cast<ClusterKey>(key >> CLUSTER_BITS);
    }

    [[nodiscard]] static Key combine(ClusterKey hi, ChildKey lo) noexcept
    {
        return (Key(hi) << CLUSTER_BITS) | Key(lo);
    }

    ClusterEntry const *find_cluster(ClusterKey hi) const noexcept
    {
        auto it = clusters_.find(hi);
        return it == clusters_.end() ? nullptr : &it->second;
    }

    void remove_cluster(typename ClusterMap::iterator it) noexcept
    {
        summary_.erase(it->first);
        clusters_.erase(it);
    }

    [[nodiscard]] static Child &ensure_child(ClusterEntry &entry)
    {
        if (!entry.child) {
            entry.child = std::make_unique<Child>();
        }
        return *entry.child;
    }

    Summary summary_{};
    ClusterMap clusters_{};
};
