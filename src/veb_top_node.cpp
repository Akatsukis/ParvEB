#include "veb_top_node.hpp"

#include <bit>
#include <span>

#include "simd_utils.hpp"

namespace {

constexpr uint64_t word_bit(unsigned idx)
{
    return uint64_t(1) << (idx & (VebTopNode::WORD_SIZE - 1));
}

constexpr unsigned word_index(unsigned idx)
{
    return idx >> VebTopNode::WORD_BITS;
}

} // namespace

bool VebTopNode::empty() const noexcept
{
    return child_count_ == 0;
}

void VebTopNode::set_occ_bit(unsigned idx) noexcept
{
    unsigned w_idx = word_index(idx);
    uint64_t bit = word_bit(idx);
    if (!(occ_words_[w_idx] & bit)) {
        occ_words_[w_idx] |= bit;
        ++child_count_;
    }
}

void VebTopNode::clear_occ_bit(unsigned idx) noexcept
{
    unsigned w_idx = word_index(idx);
    uint64_t bit = word_bit(idx);
    if (occ_words_[w_idx] & bit) {
        occ_words_[w_idx] &= ~bit;
        --child_count_;
    }
}

VebTopNode::Child *VebTopNode::get_child(unsigned idx) noexcept
{
    if (idx >= TOP_SIZE) {
        return nullptr;
    }
    return child_[idx].get();
}

VebTopNode::Child const *VebTopNode::get_child(unsigned idx) const noexcept
{
    if (idx >= TOP_SIZE) {
        return nullptr;
    }
    return child_[idx].get();
}

VebTopNode::Child &VebTopNode::ensure_child(unsigned idx)
{
    auto &ptr = child_[idx];
    if (!ptr) {
        ptr = std::make_unique<Child>();
        set_occ_bit(idx);
    }
    return *ptr;
}

std::optional<unsigned> VebTopNode::find_next_word(unsigned start_word) const noexcept
{
    auto span = std::span<const uint64_t>(occ_words_);
    auto idx = simd::find_next_nonzero(span, start_word);
    if (!idx) {
        return std::nullopt;
    }
    return static_cast<unsigned>(*idx);
}

std::optional<unsigned> VebTopNode::find_prev_word(unsigned start_word) const noexcept
{
    auto span = std::span<const uint64_t>(occ_words_);
    auto idx = simd::find_prev_nonzero(span, start_word);
    if (!idx) {
        return std::nullopt;
    }
    return static_cast<unsigned>(*idx);
}

void VebTopNode::insert(Key key)
{
    unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
    Key lo = key & CHILD_MASK;
    Child &c = ensure_child(hi);
    c.insert(lo);
}

void VebTopNode::erase(Key key) noexcept
{
    unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
    if (hi >= TOP_SIZE) {
        return;
    }
    Key lo = key & CHILD_MASK;
    Child *c = get_child(hi);
    if (!c) {
        return;
    }
    c->erase(lo);
    if (c->empty()) {
        child_[hi].reset();
        clear_occ_bit(hi);
    }
}

bool VebTopNode::contains(Key key) const noexcept
{
    unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
    if (hi >= TOP_SIZE) {
        return false;
    }
    Key lo = key & CHILD_MASK;
    Child const *c = get_child(hi);
    if (!c) {
        return false;
    }
    return c->contains(lo);
}

std::optional<VebTopNode::Key> VebTopNode::min() const noexcept
{
    if (empty()) {
        return std::nullopt;
    }
    auto word_idx = find_next_word(0);
    if (!word_idx) {
        return std::nullopt;
    }
    uint64_t word = occ_words_[*word_idx];
    unsigned hi = (*word_idx << WORD_BITS) + std::countr_zero(word);
    Child const *c = get_child(hi);
    auto lo = c ? c->min() : std::nullopt;
    if (!lo) {
        return std::nullopt;
    }
    return (Key(hi) << CHILD_BITS) | *lo;
}

std::optional<VebTopNode::Key> VebTopNode::max() const noexcept
{
    if (empty()) {
        return std::nullopt;
    }
    auto word_idx = find_prev_word(WORD_COUNT - 1);
    if (!word_idx) {
        return std::nullopt;
    }
    uint64_t word = occ_words_[*word_idx];
    unsigned hi = (*word_idx << WORD_BITS) + (63u - std::countl_zero(word));
    Child const *c = get_child(hi);
    auto lo = c ? c->max() : std::nullopt;
    if (!lo) {
        return std::nullopt;
    }
    return (Key(hi) << CHILD_BITS) | *lo;
}

std::optional<VebTopNode::Key> VebTopNode::successor(Key key) const noexcept
{
    if (empty()) {
        return std::nullopt;
    }
    unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
    Key lo = key & CHILD_MASK;
    if (hi < TOP_SIZE) {
        Child const *c = get_child(hi);
        if (c) {
            if (auto s_lo = c->successor(lo)) {
                return (Key(hi) << CHILD_BITS) | *s_lo;
            }
        }
    }
    if (hi >= TOP_SIZE - 1) {
        return std::nullopt;
    }
    unsigned word_idx = hi >> WORD_BITS;
    unsigned bit_index = hi & (WORD_SIZE - 1);
    uint64_t word = occ_words_[word_idx];
    if (bit_index < WORD_SIZE - 1) {
        uint64_t mask = word & (~0ull << (bit_index + 1));
        if (mask) {
            unsigned next_hi = (word_idx << WORD_BITS) + std::countr_zero(mask);
            Child const *c = get_child(next_hi);
            auto lo_min = c ? c->min() : std::nullopt;
            if (lo_min) {
                return (Key(next_hi) << CHILD_BITS) | *lo_min;
            }
        }
    }
    auto next_word = find_next_word(word_idx + 1);
    if (!next_word) {
        return std::nullopt;
    }
    uint64_t next_word_bits = occ_words_[*next_word];
    unsigned next_hi = (*next_word << WORD_BITS) + std::countr_zero(next_word_bits);
    Child const *c = get_child(next_hi);
    auto lo_min = c ? c->min() : std::nullopt;
    if (!lo_min) {
        return std::nullopt;
    }
    return (Key(next_hi) << CHILD_BITS) | *lo_min;
}

std::optional<VebTopNode::Key> VebTopNode::predecessor(Key key) const noexcept
{
    if (empty()) {
        return std::nullopt;
    }
    unsigned hi = static_cast<unsigned>(key >> CHILD_BITS);
    Key lo = key & CHILD_MASK;
    if (hi < TOP_SIZE) {
        Child const *c = get_child(hi);
        if (c) {
            if (auto p_lo = c->predecessor(lo)) {
                return (Key(hi) << CHILD_BITS) | *p_lo;
            }
        }
    }
    if (hi == 0) {
        return std::nullopt;
    }
    unsigned word_idx = hi >> WORD_BITS;
    unsigned bit_index = hi & (WORD_SIZE - 1);
    uint64_t mask = bit_index == 0 ? 0 : ((uint64_t(1) << bit_index) - 1);
    uint64_t word = occ_words_[word_idx] & mask;
    if (word) {
        unsigned prev_hi = (word_idx << WORD_BITS) + (63u - std::countl_zero(word));
        Child const *c = get_child(prev_hi);
        auto lo_max = c ? c->max() : std::nullopt;
        if (!lo_max) {
            return std::nullopt;
        }
        return (Key(prev_hi) << CHILD_BITS) | *lo_max;
    }
    if (word_idx == 0) {
        return std::nullopt;
    }
    auto prev_word = find_prev_word(word_idx - 1);
    if (!prev_word) {
        return std::nullopt;
    }
    uint64_t prev_bits = occ_words_[*prev_word];
    unsigned prev_hi = (*prev_word << WORD_BITS) + (63u - std::countl_zero(prev_bits));
    Child const *c = get_child(prev_hi);
    auto lo_max = c ? c->max() : std::nullopt;
    if (!lo_max) {
        return std::nullopt;
    }
    return (Key(prev_hi) << CHILD_BITS) | *lo_max;
}
