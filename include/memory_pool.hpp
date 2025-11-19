#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#ifndef PARVEB_ENABLE_POOL
    #define PARVEB_ENABLE_POOL 1
#endif

namespace pool
{

#if PARVEB_ENABLE_POOL

    template <class T> class ObjectPool
    {
        using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

    public:
        ObjectPool() = default;

        ObjectPool(ObjectPool const &) = delete;
        ObjectPool &operator=(ObjectPool const &) = delete;

        void reserve(std::size_t count)
        {
            while (free_list_.size() < count) {
                allocate_chunk(std::max(next_chunk_size_, count - free_list_.size()));
            }
        }

        template <class... Args> T *create(Args &&...args)
        {
            if (free_list_.empty()) {
                allocate_chunk(next_chunk_size_);
                next_chunk_size_ *= 2;
            }
            T *slot = free_list_.back();
            free_list_.pop_back();
            ::new (static_cast<void *>(slot)) T(std::forward<Args>(args)...);
            return slot;
        }

        void destroy(T *ptr) noexcept
        {
            if (!ptr) {
                return;
            }
            ptr->~T();
            free_list_.push_back(ptr);
        }

    private:
        void allocate_chunk(std::size_t chunk_size)
        {
            auto chunk = std::make_unique<Storage[]>(chunk_size);
            T *base = reinterpret_cast<T *>(chunk.get());
            for (std::size_t i = 0; i < chunk_size; ++i) {
                free_list_.push_back(base + i);
            }
            chunks_.push_back(std::move(chunk));
        }

        std::vector<std::unique_ptr<Storage[]>> chunks_{};
        std::vector<T *> free_list_{};
        std::size_t next_chunk_size_{1024};
    };

    template <class T> class PoolDeleter
    {
    public:
        PoolDeleter() = default;
        explicit PoolDeleter(ObjectPool<T> *pool) : pool_(pool) {}

        void operator()(T *ptr) const noexcept
        {
            if (ptr && pool_) {
                pool_->destroy(ptr);
            }
        }

    private:
        ObjectPool<T> *pool_{nullptr};
    };

    template <class T, class... Args>
    std::unique_ptr<T, PoolDeleter<T>>
    make_unique(ObjectPool<T> &pool, Args &&...args)
    {
        return std::unique_ptr<T, PoolDeleter<T>>(
            pool.create(std::forward<Args>(args)...), PoolDeleter<T>(&pool));
    }

#else

    template <class T> class ObjectPool
    {
    public:
        ObjectPool() = default;
        ObjectPool(ObjectPool const &) = delete;
        ObjectPool &operator=(ObjectPool const &) = delete;

        void reserve(std::size_t) {}

        template <class... Args> T *create(Args &&...args)
        {
            return new T(std::forward<Args>(args)...);
        }

        void destroy(T *ptr) noexcept
        {
            delete ptr;
        }
    };

    template <class T> struct PoolDeleter
    {
        void operator()(T *ptr) const noexcept
        {
            delete ptr;
        }
    };

    template <class T, class... Args>
    std::unique_ptr<T, PoolDeleter<T>>
    make_unique(ObjectPool<T> &, Args &&...args)
    {
        return std::unique_ptr<T, PoolDeleter<T>>(
            new T(std::forward<Args>(args)...), PoolDeleter<T>());
    }

#endif

} // namespace pool
