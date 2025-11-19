#pragma once

#include <cassert>
#include <optional>
#include <vector>

#include "veb_top32.hpp"

class VebTree32
{
public:
    using Key = VebTop32::Key;
    static constexpr Key MAX_KEY = VebTop32::MAX_KEY;
    static constexpr uint64_t PREDECESSOR_QUERY_MAX =
        VebTop32::PREDECESSOR_QUERY_MAX;

    VebTree32()
        : pools_(std::make_shared<VebMemoryPools>()), root_(pools_.get())
    {
    }

    explicit VebTree32(std::shared_ptr<VebMemoryPools> pools)
        : pools_(std::move(pools)), root_(pools_ ? pools_.get() : nullptr)
    {
    }

    void reserve(std::size_t branch_nodes, std::size_t leaf_nodes)
    {
        if (!pools_) {
            return;
        }
        pools_->branch_pool.reserve(branch_nodes);
        pools_->leaf_pool.reserve(leaf_nodes);
    }

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

    std::optional<Key> predecessor(uint64_t key) const noexcept
    {
        assert(key <= PREDECESSOR_QUERY_MAX);
        return root_.predecessor(key);
    }

    template <class Fn> void for_each(Fn &&fn) const
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
    std::shared_ptr<VebMemoryPools> pools_;
    VebTop32 root_;
};
