#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace simd
{

    [[nodiscard]] std::optional<std::size_t> find_next_nonzero(
        std::span<uint64_t const> words, std::size_t start_word) noexcept;

    [[nodiscard]] std::optional<std::size_t> find_prev_nonzero(
        std::span<uint64_t const> words, std::size_t start_word) noexcept;

} // namespace simd
