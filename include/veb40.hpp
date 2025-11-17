#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "veb_top_node.hpp"

template <class RootNode, unsigned EFFECTIVE_BITS>
class VanEmdeBoasTree
{
    static_assert(EFFECTIVE_BITS > 0, "EFFECTIVE_BITS must be > 0");
    static_assert(
        RootNode::SUBTREE_BITS >= EFFECTIVE_BITS,
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
    static constexpr Key PREDECESSOR_QUERY_MAX =
        compute_predecessor_query_max();

    bool empty() const noexcept
    {
        return root_.empty();
    }

    void insert(Key key)
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

    template <class Fn>
    void for_each(Fn &&fn) const
    {
        root_.for_each(std::forward<Fn>(fn));
    }

    std::vector<Key> to_vector() const
    {
        std::vector<Key> out;
        root_.for_each([&](Key k) { out.push_back(k); });
        return out;
    }

private:
    RootNode root_{};
};

using VebTree40 = VanEmdeBoasTree<VebTopNode, 40>;
