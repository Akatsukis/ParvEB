#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "quill/Quill.h"

template <typename Duration = std::chrono::nanoseconds>
class Stopwatch final
{
    using clock = std::chrono::steady_clock;
    using time_point = typename clock::time_point;

    bool on_{false};
    std::string name_;
    time_point last_{};
    Duration total_{Duration::zero()};

    Duration next_time() noexcept
    {
        if (!on_) {
            return Duration::zero();
        }

        std::atomic_thread_fence(std::memory_order_acquire);

        auto const now = clock::now();

        std::atomic_thread_fence(std::memory_order_release);

        auto const elapsed = std::chrono::duration_cast<Duration>(now - last_);
        total_ += elapsed;
        last_ = now;
        return elapsed;
    }

public:
    explicit Stopwatch(std::string name = "vEB", bool start_running = true)
        : name_(std::move(name))
    {
        if (start_running) {
            LOG_INFO("Starting stopwatch {}", name_);
            start();
        }
    }

    void start() noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);

        last_ = clock::now();
        on_ = true;

        std::atomic_thread_fence(std::memory_order_release);
    }

    void stop() noexcept
    {
        if (on_) {
            next_time();
        }
        std::atomic_thread_fence(std::memory_order_release);

        on_ = false;
    }

    void report(Duration elapsed, std::string_view msg = "") const
    {
        auto const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
        if (ns >= std::chrono::seconds(100)) {
            LOG_INFO(
                "Stopwatch {}: {} elapsed time: {}",
                name_,
                msg,
                std::chrono::duration_cast<std::chrono::seconds>(ns));
        }
        else if (ns >= std::chrono::milliseconds(100)) {
            LOG_INFO(
                "Stopwatch {}: {} elapsed time: {}",
                name_,
                msg,
                std::chrono::duration_cast<std::chrono::milliseconds>(ns));
        }
        else if (ns >= std::chrono::microseconds(100)) {
            LOG_INFO(
                "Stopwatch {}: {} elapsed time: {}",
                name_,
                msg,
                std::chrono::duration_cast<std::chrono::microseconds>(ns));
        }
        else {
            LOG_INFO("Stopwatch {}: {} elapsed time: {}", name_, msg, ns);
        }
    }

    void next(std::string_view label)
    {
        if (on_) {
            report(next_time(), label);
        }
    }

    void total_time() const
    {
        report(total_, "total");
    }

    [[nodiscard]] Duration total_elapsed() const noexcept
    {
        return total_;
    }
};
