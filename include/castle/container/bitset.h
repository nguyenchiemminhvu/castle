#ifndef CASTLE_CONTAINER_BITSET_H
#define CASTLE_CONTAINER_BITSET_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/error/status.h"
#include <stdint.h>

namespace castle
{
namespace container
{

// Fixed-size bitset. Bits are numbered from least-significant bit position 0.
template <size_type N>
class bitset
{
    static_assert(N > 0U, "bitset requires a positive bit count");

    static CASTLE_CONSTEXPR size_type word_bits = 32U;
    static CASTLE_CONSTEXPR size_type word_count = (N + word_bits - 1U) / word_bits;

public:
    class reference
    {
    public:
        reference(uint32_t& word, uint32_t mask) CASTLE_NOEXCEPT
            : word_(&word), mask_(mask)
        {
        }

        reference& operator=(bool value) CASTLE_NOEXCEPT
        {
            if (value)
            {
                *word_ |= mask_;
            }
            else
            {
                *word_ &= ~mask_;
            }
            return *this;
        }

        reference& operator=(CASTLE_CONST reference& other) CASTLE_NOEXCEPT
        {
            return (*this = static_cast<bool>(other));
        }

        operator bool() const CASTLE_NOEXCEPT
        {
            return (*word_ & mask_) != 0U;
        }

        void flip() CASTLE_NOEXCEPT
        {
            *word_ ^= mask_;
        }

    private:
        uint32_t* word_;
        uint32_t mask_;
    };

    bitset() CASTLE_NOEXCEPT : words_{}
    {
    }

    explicit bitset(uint64_t value) CASTLE_NOEXCEPT : words_{}
    {
        words_[0U] = static_cast<uint32_t>(value);
        if (word_count > 1U)
        {
            words_[1U] = static_cast<uint32_t>(value >> 32U);
        }
        sanitize_top_word();
    }

    size_type size() const CASTLE_NOEXCEPT { return N; }

    reference operator[](size_type position) CASTLE_NOEXCEPT
    {
        return reference(words_[position / word_bits], mask(position));
    }

    bool operator[](size_type position) const CASTLE_NOEXCEPT
    {
        return test(position);
    }

    status set(size_type position, bool value = true) CASTLE_NOEXCEPT
    {
        if (position >= N)
        {
            return status::out_of_range;
        }
        reference(words_[position / word_bits], mask(position)) = value;
        return status::ok;
    }

    void set() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = 0xFFFFFFFFU;
        }
        sanitize_top_word();
    }

    status reset(size_type position) CASTLE_NOEXCEPT
    {
        if (position >= N)
        {
            return status::out_of_range;
        }
        words_[position / word_bits] &= ~mask(position);
        return status::ok;
    }

    void reset() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = 0U;
        }
    }

    status flip(size_type position) CASTLE_NOEXCEPT
    {
        if (position >= N)
        {
            return status::out_of_range;
        }
        words_[position / word_bits] ^= mask(position);
        return status::ok;
    }

    void flip() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = ~words_[i];
        }
        sanitize_top_word();
    }

    bool test(size_type position) const CASTLE_NOEXCEPT
    {
        if (position >= N)
        {
            return false;
        }
        return (words_[position / word_bits] & mask(position)) != 0U;
    }

    size_type count() const CASTLE_NOEXCEPT
    {
        size_type result = 0U;
        for (size_type i = 0U; i < word_count; ++i)
        {
            result += popcount32(words_[i]);
        }
        return result;
    }

    bool any() const CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            if (words_[i] != 0U)
            {
                return true;
            }
        }
        return false;
    }

    bool none() const CASTLE_NOEXCEPT
    {
        return !any();
    }

    bool all() const CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i + 1U < word_count; ++i)
        {
            if (words_[i] != 0xFFFFFFFFU)
            {
                return false;
            }
        }
        return (words_[word_count - 1U] & top_mask()) == top_mask();
    }

    uint32_t to_uint32() const CASTLE_NOEXCEPT
    {
        return words_[0U];
    }

    uint64_t to_uint64() const CASTLE_NOEXCEPT
    {
        uint64_t value = static_cast<uint64_t>(words_[0U]);
        if (word_count > 1U)
        {
            value |= static_cast<uint64_t>(words_[1U]) << 32U;
        }
        return value;
    }

    status to_string(char* dst, size_type capacity) const CASTLE_NOEXCEPT
    {
        if ((dst == nullptr) || (capacity < (N + 1U)))
        {
            return status::invalid_argument;
        }
        for (size_type i = 0U; i < N; ++i)
        {
            const size_type bit = N - 1U - i;
            dst[i] = test(bit) ? '1' : '0';
        }
        dst[N] = '\0';
        return status::ok;
    }

    bitset& operator&=(CASTLE_CONST bitset& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] &= other.words_[i];
        }
        return *this;
    }

    bitset& operator|=(CASTLE_CONST bitset& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] |= other.words_[i];
        }
        sanitize_top_word();
        return *this;
    }

    bitset& operator^=(CASTLE_CONST bitset& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] ^= other.words_[i];
        }
        sanitize_top_word();
        return *this;
    }

    bitset operator~() const CASTLE_NOEXCEPT
    {
        bitset result(*this);
        result.flip();
        return result;
    }

private:
    static CASTLE_CONSTEXPR uint32_t mask(size_type position) CASTLE_NOEXCEPT
    {
        return static_cast<uint32_t>(1UL << (position % word_bits));
    }

    static CASTLE_CONSTEXPR uint32_t top_mask() CASTLE_NOEXCEPT
    {
        return (N % word_bits) == 0U
               ? 0xFFFFFFFFU
               : static_cast<uint32_t>((1UL << (N % word_bits)) - 1UL);
    }

    static size_type popcount32(uint32_t value) CASTLE_NOEXCEPT
    {
        value = value - ((value >> 1U) & 0x55555555U);
        value = (value & 0x33333333U) + ((value >> 2U) & 0x33333333U);
        value = (value + (value >> 4U)) & 0x0F0F0F0FU;
        const uint32_t packed = value * 0x01010101U;
        return static_cast<size_type>(packed >> 24U);
    }

    void sanitize_top_word() CASTLE_NOEXCEPT
    {
        words_[word_count - 1U] &= top_mask();
    }

    uint32_t words_[word_count];
};

template <size_type N>
bitset<N> operator&(bitset<N> lhs, CASTLE_CONST bitset<N>& rhs) CASTLE_NOEXCEPT
{
    lhs &= rhs;
    return lhs;
}

template <size_type N>
bitset<N> operator|(bitset<N> lhs, CASTLE_CONST bitset<N>& rhs) CASTLE_NOEXCEPT
{
    lhs |= rhs;
    return lhs;
}

template <size_type N>
bitset<N> operator^(bitset<N> lhs, CASTLE_CONST bitset<N>& rhs) CASTLE_NOEXCEPT
{
    lhs ^= rhs;
    return lhs;
}

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_BITSET_H
