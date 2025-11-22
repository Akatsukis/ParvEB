#pragma once

#include <cassert>
#include <optional>
#include <utility>
#include <vector>

#include "veb_branch.hpp"

class VebTree64
{
public:
    using Key = VebTop64::Key;
    static constexpr Key MAX_KEY = VebTop64::MAX_KEY;
    static constexpr Key PREDECESSOR_QUERY_MAX =
        VebTop64::PREDECESSOR_QUERY_MAX;

    VebTree64() = default;

    bool empty() const noexcept
    {
        return root_.empty();
    }

    void insert(Key key)
    {
        root_.insert(key);
    }

    void erase(Key key) noexcept
    {
        root_.erase(key);
    }

    bool contains(Key key) const noexcept
    {
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
    VebTop64 root_{};
};
