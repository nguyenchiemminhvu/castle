#ifndef CASTLE_UTILITY_BITSET_H
#define CASTLE_UTILITY_BITSET_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/core/traits.h"

#include <limits.h>
#include <stdint.h>

namespace castle
{

// ============================================================================
// bitset
//
// Fixed-size bit storage for embedded targets.
//
// The second template parameter is an intentional embedded extension. It keeps
// the std::bitset-like spelling usable while allowing the storage word to match
// the native register width of the target MCU.
// ============================================================================
template <size_type Bits, typename Word = uint32_t>
class bitset
{
    static_assert(meta::is_integral<Word>::value,
                  "castle::bitset storage word must be an integral type");
    static_assert(meta::is_unsigned<Word>::value,
                  "castle::bitset storage word must be unsigned");
    static_assert(!meta::is_same<Word, bool>::value,
                  "castle::bitset storage word must not be bool");

public:
    using value_type = bool;
    using word_type = Word;
    using size_type = castle::size_type;

private:
    static CASTLE_CONST size_type word_bits = sizeof(word_type) * CHAR_BIT;
    static CASTLE_CONST size_type word_count = (Bits == 0U) ? 0U : ((Bits + word_bits - 1U) / word_bits);
    static CASTLE_CONST size_type storage_count = (word_count == 0U) ? 1U : word_count;

    static CASTLE_CONSTEXPR word_type all_ones() CASTLE_NOEXCEPT
    {
        return static_cast<word_type>(~static_cast<word_type>(0));
    }

    static CASTLE_CONSTEXPR word_type top_mask_value() CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type remainder = Bits % word_bits;
        return (Bits == 0U)
            ? static_cast<word_type>(0)
            : ((remainder == 0U)
                ? all_ones()
                : static_cast<word_type>((static_cast<word_type>(1U) << remainder) - static_cast<word_type>(1U)));
    }

    void trim_unused_bits() CASTLE_NOEXCEPT
    {
        if (word_count != 0U)
        {
            words_[word_count - 1U] &= top_mask_value();
        }
    }

    static size_type word_index(size_type position) CASTLE_NOEXCEPT
    {
        return position / word_bits;
    }

    static word_type bit_mask(size_type position) CASTLE_NOEXCEPT
    {
        return static_cast<word_type>(static_cast<word_type>(1U) << (position % word_bits));
    }

public:
    // ========================================================================
    // reference
    //
    // Proxy returned by non-CASTLE_CONST operator[]. It stores only a pointer to the
    // owning bitset and a bit position; no heap allocation is involved.
    // ========================================================================
    class reference
    {
    public:
        reference(bitset& owner, size_type position) CASTLE_NOEXCEPT
            : owner_(&owner)
            , position_(position)
        {
        }

        reference& operator=(bool value) CASTLE_NOEXCEPT
        {
            owner_->set(position_, value);
            return *this;
        }

        reference& operator=(CASTLE_CONST reference& other) CASTLE_NOEXCEPT
        {
            owner_->set(position_, static_cast<bool>(other));
            return *this;
        }

        operator bool() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return owner_->test(position_);
        }

        bool operator~() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return !owner_->test(position_);
        }

        reference& flip() CASTLE_NOEXCEPT
        {
            owner_->flip(position_);
            return *this;
        }

    private:
        bitset* owner_;
        size_type position_;
    };

    // ------------------------------------------------------------------------
    // Construction.
    // ------------------------------------------------------------------------
    CASTLE_CONSTEXPR bitset() CASTLE_NOEXCEPT
        : words_{0}
    {
    }

    explicit bitset(unsigned long long value) CASTLE_NOEXCEPT
        : words_{0}
    {
        assign(value);
    }

    explicit bitset(CASTLE_CONST char* text) CASTLE_NOEXCEPT
        : words_{0}
    {
        from_string(text);
    }

    bitset(CASTLE_CONST bitset&) CASTLE_DEFAULT;
    bitset(bitset&&) CASTLE_DEFAULT;

    bitset& operator=(CASTLE_CONST bitset&) CASTLE_DEFAULT;
    bitset& operator=(bitset&&) CASTLE_DEFAULT;

    // ------------------------------------------------------------------------
    // Capacity.
    // ------------------------------------------------------------------------
    static CASTLE_CONSTEXPR size_type size() CASTLE_NOEXCEPT
    {
        return Bits;
    }

    static CASTLE_CONSTEXPR size_type number_of_words() CASTLE_NOEXCEPT
    {
        return word_count;
    }

    static CASTLE_CONSTEXPR size_type bits_per_word() CASTLE_NOEXCEPT
    {
        return word_bits;
    }

    // ------------------------------------------------------------------------
    // Raw storage access.
    //
    // Useful when a bitset is used as a register/mask image. Only the first
    // number_of_words() elements are logically part of the bitset.
    // ------------------------------------------------------------------------
    word_type* data() CASTLE_NOEXCEPT
    {
        return words_;
    }

    CASTLE_CONST word_type* data() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return words_;
    }

    // ------------------------------------------------------------------------
    // State queries.
    // ------------------------------------------------------------------------
    bool test(size_type position) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (position >= Bits)
        {
            return false;
        }

        return (words_[word_index(position)] & bit_mask(position)) != static_cast<word_type>(0);
    }

    CASTLE_NODISCARD bool any() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            if (words_[i] != static_cast<word_type>(0))
            {
                return true;
            }
        }
        return false;
    }

    CASTLE_NODISCARD bool none() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !any();
    }

    CASTLE_NODISCARD bool all() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (Bits == 0U)
        {
            return true;
        }

        for (size_type i = 0U; i + 1U < word_count; ++i)
        {
            if (words_[i] != all_ones())
            {
                return false;
            }
        }

        return words_[word_count - 1U] == top_mask_value();
    }

    CASTLE_NODISCARD size_type count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type result = 0U;

        for (size_type i = 0U; i < word_count; ++i)
        {
            word_type value = words_[i];
            while (value != static_cast<word_type>(0))
            {
                value &= static_cast<word_type>(value - static_cast<word_type>(1U));
                ++result;
            }
        }

        return result;
    }

    static CASTLE_CONSTEXPR size_type npos() CASTLE_NOEXCEPT
    {
        return Bits;
    }

    // ------------------------------------------------------------------------
    // Modification.
    // ------------------------------------------------------------------------
    bitset& set() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = all_ones();
        }
        trim_unused_bits();
        return *this;
    }

    bitset& set(size_type position, bool value = true) CASTLE_NOEXCEPT
    {
        if (position >= Bits)
        {
            return *this;
        }

        word_type& word = words_[word_index(position)];
        CASTLE_CONST word_type mask = bit_mask(position);
        if (value)
        {
            word |= mask;
        }
        else
        {
            word &= static_cast<word_type>(~mask);
        }
        return *this;
    }

    bitset& reset() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = static_cast<word_type>(0);
        }
        return *this;
    }

    bitset& reset(size_type position) CASTLE_NOEXCEPT
    {
        return set(position, false);
    }

    bitset& flip() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = static_cast<word_type>(~words_[i]);
        }
        trim_unused_bits();
        return *this;
    }

    bitset& flip(size_type position) CASTLE_NOEXCEPT
    {
        if (position >= Bits)
        {
            return *this;
        }

        words_[word_index(position)] ^= bit_mask(position);
        return *this;
    }

    bitset& assign(unsigned long long value) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = static_cast<word_type>(value);
            if (sizeof(unsigned long long) * CHAR_BIT <= (i + 1U) * word_bits)
            {
                value = 0ULL;
            }
            else
            {
                value >>= word_bits;
            }
        }
        trim_unused_bits();
        return *this;
    }

    bitset& from_string(CASTLE_CONST char* text,
                        size_type position = 0U,
                        size_type length = static_cast<size_type>(-1),
                        char zero = '0',
                        char one = '1') CASTLE_NOEXCEPT
    {
        reset();

        if (text == 0)
        {
            return *this;
        }

        size_type available = 0U;
        while (available < length && text[position + available] != '\0')
        {
            ++available;
        }

        CASTLE_CONST size_type used = (available < Bits) ? available : Bits;
        for (size_type i = 0U; i < used; ++i)
        {
            CASTLE_CONST char c = text[position + available - 1U - i];
            if (c == one)
            {
                words_[word_index(i)] |= bit_mask(i);
            }
            else if (c == zero)
            {
                // Already reset.
            }
            else
            {
                // Embedded-friendly policy: invalid input leaves that bit clear.
            }
        }

        return *this;
    }

    // ------------------------------------------------------------------------
    // Integral conversion.
    //
    // These functions intentionally use a compile-time size check instead of
    // throwing std::overflow_error. This keeps misuse visible during build and
    // keeps the runtime path deterministic for exception-free targets.
    // ------------------------------------------------------------------------
    unsigned long to_ulong() CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(Bits <= sizeof(unsigned long) * CHAR_BIT,
                      "castle::bitset::to_ulong() target type is too small");
        unsigned long result = 0UL;
        for (size_type i = 0U; i < Bits; ++i)
        {
            if (test(i))
            {
                result |= (static_cast<unsigned long>(1UL) << i);
            }
        }
        return result;
    }

    unsigned long long to_ullong() CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(Bits <= sizeof(unsigned long long) * CHAR_BIT,
                      "castle::bitset::to_ullong() target type is too small");
        unsigned long long result = 0ULL;
        for (size_type i = 0U; i < Bits; ++i)
        {
            if (test(i))
            {
                result |= (static_cast<unsigned long long>(1ULL) << i);
            }
        }
        return result;
    }

    // ------------------------------------------------------------------------
    // String conversion without requiring castle::string.
    //
    // The caller owns the destination buffer. On success, exactly Bits
    // characters plus a trailing '\0' are written. The buffer is untouched when
    // it is null or too small.
    // ------------------------------------------------------------------------
    bool to_string(char* buffer,
                   size_type capacity,
                   char zero = '0',
                   char one = '1') CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (buffer == 0 || capacity < Bits + 1U)
        {
            return false;
        }

        for (size_type i = 0U; i < Bits; ++i)
        {
            buffer[Bits - 1U - i] = test(i) ? one : zero;
        }
        buffer[Bits] = '\0';
        return true;
    }

    // ------------------------------------------------------------------------
    // Search.
    // ------------------------------------------------------------------------
    size_type find_first(bool state) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Bits; ++i)
        {
            if (test(i) == state)
            {
                return i;
            }
        }
        return npos();
    }

    size_type find_next(size_type position, bool state) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (position >= Bits)
        {
            return npos();
        }

        for (size_type i = position + 1U; i < Bits; ++i)
        {
            if (test(i) == state)
            {
                return i;
            }
        }
        return npos();
    }

    // ------------------------------------------------------------------------
    // Element access.
    // ------------------------------------------------------------------------
    reference operator[](size_type position) CASTLE_NOEXCEPT
    {
        return reference(*this, position);
    }

    bool operator[](size_type position) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return test(position);
    }

    // ------------------------------------------------------------------------
    // Compound bitwise operations.
    // ------------------------------------------------------------------------
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
        return *this;
    }

    bitset& operator^=(CASTLE_CONST bitset& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] ^= other.words_[i];
        }
        trim_unused_bits();
        return *this;
    }

    bitset& operator<<=(size_type shift) CASTLE_NOEXCEPT
    {
        if (shift >= Bits)
        {
            return reset();
        }

        if (shift == 0U || word_count == 0U)
        {
            return *this;
        }

        CASTLE_CONST size_type word_shift = shift / word_bits;
        CASTLE_CONST size_type bit_shift = shift % word_bits;

        for (size_type i = word_count; i > 0U; --i)
        {
            CASTLE_CONST size_type destination = i - 1U;
            word_type value = static_cast<word_type>(0);

            if (destination >= word_shift)
            {
                CASTLE_CONST size_type source = destination - word_shift;
                value = static_cast<word_type>(words_[source] << bit_shift);

                if (bit_shift != 0U && source > 0U)
                {
                    value |= static_cast<word_type>(words_[source - 1U] >> (word_bits - bit_shift));
                }
            }

            words_[destination] = value;
        }

        trim_unused_bits();
        return *this;
    }

    bitset& operator>>=(size_type shift) CASTLE_NOEXCEPT
    {
        if (shift >= Bits)
        {
            return reset();
        }

        if (shift == 0U || word_count == 0U)
        {
            return *this;
        }

        CASTLE_CONST size_type word_shift = shift / word_bits;
        CASTLE_CONST size_type bit_shift = shift % word_bits;

        for (size_type destination = 0U; destination < word_count; ++destination)
        {
            word_type value = static_cast<word_type>(0);
            CASTLE_CONST size_type source = destination + word_shift;

            if (source < word_count)
            {
                value = static_cast<word_type>(words_[source] >> bit_shift);

                if (bit_shift != 0U && source + 1U < word_count)
                {
                    value |= static_cast<word_type>(words_[source + 1U] << (word_bits - bit_shift));
                }
            }

            words_[destination] = value;
        }

        trim_unused_bits();
        return *this;
    }

    // ------------------------------------------------------------------------
    // Swap.
    // ------------------------------------------------------------------------
    void swap(bitset& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            CASTLE_CONST word_type temporary = words_[i];
            words_[i] = other.words_[i];
            other.words_[i] = temporary;
        }
    }

private:
    word_type words_[storage_count];
};

// ============================================================================
// Non-member operators.
// ============================================================================
template <size_type Bits, typename Word>
bitset<Bits, Word> operator&(bitset<Bits, Word> lhs,
                             CASTLE_CONST bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    lhs &= rhs;
    return lhs;
}

template <size_type Bits, typename Word>
bitset<Bits, Word> operator|(bitset<Bits, Word> lhs,
                             CASTLE_CONST bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    lhs |= rhs;
    return lhs;
}

template <size_type Bits, typename Word>
bitset<Bits, Word> operator^(bitset<Bits, Word> lhs,
                             CASTLE_CONST bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    lhs ^= rhs;
    return lhs;
}

template <size_type Bits, typename Word>
bitset<Bits, Word> operator<<(bitset<Bits, Word> lhs,
                              size_type shift) CASTLE_NOEXCEPT
{
    lhs <<= shift;
    return lhs;
}

template <size_type Bits, typename Word>
bitset<Bits, Word> operator>>(bitset<Bits, Word> lhs,
                              size_type shift) CASTLE_NOEXCEPT
{
    lhs >>= shift;
    return lhs;
}

template <size_type Bits, typename Word>
bitset<Bits, Word> operator~(CASTLE_CONST bitset<Bits, Word>& value) CASTLE_NOEXCEPT
{
    bitset<Bits, Word> result(value);
    result.flip();
    return result;
}

template <size_type Bits, typename Word>
bool operator==(CASTLE_CONST bitset<Bits, Word>& lhs,
                CASTLE_CONST bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    for (size_type i = 0U; i < bitset<Bits, Word>::number_of_words(); ++i)
    {
        if (lhs.data()[i] != rhs.data()[i])
        {
            return false;
        }
    }
    return true;
}

template <size_type Bits, typename Word>
bool operator!=(CASTLE_CONST bitset<Bits, Word>& lhs,
                CASTLE_CONST bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

template <size_type Bits, typename Word>
void swap(bitset<Bits, Word>& lhs,
          bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    lhs.swap(rhs);
}

} // namespace castle

#endif // CASTLE_UTILITY_BITSET_H
