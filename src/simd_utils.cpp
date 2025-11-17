#include "simd_utils.hpp"

#include <algorithm>
#include <optional>
#include <span>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
    #define PARVEB_HAS_X86 1
    #include <immintrin.h>
#else
    #define PARVEB_HAS_X86 0
#endif

namespace
{

    enum class Mode
    {
        Scalar,
        SSE2,
        AVX2
    };

    Mode detect_mode() noexcept
    {
#if PARVEB_HAS_X86 && defined(__GNUC__)
        if (__builtin_cpu_supports("avx2")) {
            return Mode::AVX2;
        }
        if (__builtin_cpu_supports("sse2")) {
            return Mode::SSE2;
        }
#endif
        return Mode::Scalar;
    }

    std::optional<std::size_t> scalar_find_next(
        std::span<uint64_t const> words, std::size_t start) noexcept
    {
        for (std::size_t i = start; i < words.size(); ++i) {
            if (words[i]) {
                return i;
            }
        }
        return std::nullopt;
    }

    std::optional<std::size_t> scalar_find_prev(
        std::span<uint64_t const> words, std::size_t start) noexcept
    {
        if (words.empty()) {
            return std::nullopt;
        }
        if (start >= words.size()) {
            start = words.size() - 1;
        }
        for (std::size_t i = start + 1; i-- > 0;) {
            if (words[i]) {
                return i;
            }
            if (i == 0) {
                break;
            }
        }
        return std::nullopt;
    }

#if PARVEB_HAS_X86 && (defined(__GNUC__) || defined(__clang__))
    #define PARVEB_TARGET_AVX2 __attribute__((target("avx2")))
    #define PARVEB_TARGET_SSE2 __attribute__((target("sse2")))
#else
    #define PARVEB_TARGET_AVX2
    #define PARVEB_TARGET_SSE2
#endif

#if PARVEB_HAS_X86
    PARVEB_TARGET_AVX2 std::optional<std::size_t>
    avx2_find_next(std::span<const uint64_t> words, std::size_t start) noexcept
    {
        std::size_t i = start;
        constexpr std::size_t kStride = 4;
        for (; i + kStride <= words.size(); i += kStride) {
            __m256i chunk = _mm256_loadu_si256(
                reinterpret_cast<__m256i const *>(words.data() + i));
            if (!_mm256_testz_si256(chunk, chunk)) {
                for (std::size_t off = 0; off < kStride; ++off) {
                    if (words[i + off]) {
                        return i + off;
                    }
                }
            }
        }
        for (; i < words.size(); ++i) {
            if (words[i]) {
                return i;
            }
        }
        return std::nullopt;
    }

    PARVEB_TARGET_AVX2 std::optional<std::size_t>
    avx2_find_prev(std::span<uint64_t const> words, std::size_t start) noexcept
    {
        if (words.empty()) {
            return std::nullopt;
        }
        if (start >= words.size()) {
            start = words.size() - 1;
        }
        std::size_t idx = start;
        while ((idx + 1) % 4 != 0) {
            if (words[idx]) {
                return idx;
            }
            if (idx == 0) {
                return std::nullopt;
            }
            --idx;
        }
        while (true) {
            std::size_t base = idx + 1 >= 4 ? idx + 1 - 4 : 0;
            __m256i chunk = _mm256_loadu_si256(
                reinterpret_cast<__m256i const *>(words.data() + base));
            if (!_mm256_testz_si256(chunk, chunk)) {
                for (int off = 3; off >= 0; --off) {
                    std::size_t word_idx = base + static_cast<std::size_t>(off);
                    if (word_idx > start) {
                        continue;
                    }
                    if (words[word_idx]) {
                        return word_idx;
                    }
                }
            }
            if (base == 0) {
                break;
            }
            idx = base - 1;
        }
        for (std::size_t tail = std::min<std::size_t>(idx, 3);; --tail) {
            if (words[tail]) {
                return tail;
            }
            if (tail == 0) {
                break;
            }
        }
        return std::nullopt;
    }

    PARVEB_TARGET_SSE2 std::optional<std::size_t>
    sse2_find_next(std::span<uint64_t const> words, std::size_t start) noexcept
    {
        std::size_t i = start;
        constexpr std::size_t kStride = 2;
        __m128i const zero = _mm_setzero_si128();
        for (; i + kStride <= words.size(); i += kStride) {
            __m128i chunk = _mm_loadu_si128(
                reinterpret_cast<__m128i const *>(words.data() + i));
            __m128i cmp = _mm_cmpeq_epi8(chunk, zero);
            if (_mm_movemask_epi8(cmp) != 0xFFFF) {
                for (std::size_t off = 0; off < kStride; ++off) {
                    if (words[i + off]) {
                        return i + off;
                    }
                }
            }
        }
        for (; i < words.size(); ++i) {
            if (words[i]) {
                return i;
            }
        }
        return std::nullopt;
    }

    PARVEB_TARGET_SSE2 std::optional<std::size_t>
    sse2_find_prev(std::span<uint64_t const> words, std::size_t start) noexcept
    {
        if (words.empty()) {
            return std::nullopt;
        }
        if (start >= words.size()) {
            start = words.size() - 1;
        }
        std::size_t idx = start;
        while ((idx + 1) % 2 != 0) {
            if (words[idx]) {
                return idx;
            }
            if (idx == 0) {
                return std::nullopt;
            }
            --idx;
        }
        while (true) {
            std::size_t base = idx + 1 >= 2 ? idx + 1 - 2 : 0;
            __m128i chunk = _mm_loadu_si128(
                reinterpret_cast<__m128i const *>(words.data() + base));
            __m128i cmp = _mm_cmpeq_epi8(chunk, _mm_setzero_si128());
            if (_mm_movemask_epi8(cmp) != 0xFFFF) {
                for (int off = 1; off >= 0; --off) {
                    std::size_t word_idx = base + static_cast<std::size_t>(off);
                    if (word_idx > start) {
                        continue;
                    }
                    if (words[word_idx]) {
                        return word_idx;
                    }
                }
            }
            if (base == 0) {
                break;
            }
            idx = base - 1;
        }
        for (std::size_t tail = std::min<std::size_t>(idx, 1);; --tail) {
            if (words[tail]) {
                return tail;
            }
            if (tail == 0) {
                break;
            }
        }
        return std::nullopt;
    }
#endif

    Mode runtime_mode() noexcept
    {
        static Mode mode = detect_mode();
        return mode;
    }

} // namespace

namespace simd
{

    std::optional<std::size_t> find_next_nonzero(
        std::span<uint64_t const> words, std::size_t start_word) noexcept
    {
        if (start_word >= words.size()) {
            return std::nullopt;
        }
        switch (runtime_mode()) {
#if PARVEB_HAS_X86
        case Mode::AVX2:
            return avx2_find_next(words, start_word);
        case Mode::SSE2:
            return sse2_find_next(words, start_word);
#endif
        case Mode::Scalar:
        default:
            return scalar_find_next(words, start_word);
        }
    }

    std::optional<std::size_t> find_prev_nonzero(
        std::span<uint64_t const> words, std::size_t start_word) noexcept
    {
        if (words.empty()) {
            return std::nullopt;
        }
        switch (runtime_mode()) {
#if PARVEB_HAS_X86
        case Mode::AVX2:
            return avx2_find_prev(words, start_word);
        case Mode::SSE2:
            return sse2_find_prev(words, start_word);
#endif
        case Mode::Scalar:
        default:
            return scalar_find_prev(words, start_word);
        }
    }

} // namespace simd
