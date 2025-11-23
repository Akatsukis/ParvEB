#pragma once

#include <cassert>
#include <optional>
#include <vector>

#include "veb_branch.hpp"

class VebTree24
{
public:
    using Key = VebTop24::Key;
    static constexpr unsigned SUBTREE_BITS = VebTop24::SUBTREE_BITS;
    static constexpr Key MAX_KEY = VebTop24::MAX_KEY;

    VebTree24() = default;

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
    VebTop24 root_{};
};
