// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bitset.hpp
 * @brief Fixed-size bit container with configurable unsigned storage words.
 *
 * Use this header when flags, register maps, or protocol fields need
 * deterministic bit storage with compile-time capacity. All storage lives
 * inside the object, no allocation occurs, and operations remain exception-
 * free and STL-free.
 *
 * @code
 * castle::bitset<8U> flags(0x12U);
 * flags.set(0U).flip(4U);
 * @endcode
 */
#ifndef CASTLE_UTILITY_BITSET_HPP
#define CASTLE_UTILITY_BITSET_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"

#include <limits.h>
#include <stdint.h>

namespace castle
{

/**
 * @brief Stores `Bits` boolean flags inside an unsigned word array.
 * @tparam Bits Number of addressable bits.
 * @tparam Word Unsigned integral storage word type.
 */
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

    /**
     * @brief Returns a word with all bits set to one.
     * @return A word with all bits set to one.
     * @note This is useful for initializing all bits to a known state.
     */
    static CASTLE_CONSTEXPR word_type all_ones() CASTLE_NOEXCEPT
    {
        return static_cast<word_type>(~static_cast<word_type>(0));
    }

    /**
     * @brief Returns a word with only the top (most significant) bits set according to the number of addressable bits.
     * @return A word with only the top (most significant) bits set according to the number of addressable bits.
     * @note This is useful for masking out unused bits in the last storage word.
     */
    static CASTLE_CONSTEXPR word_type top_mask_value() CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type remainder = Bits % word_bits;
        return (Bits == 0U)
            ? static_cast<word_type>(0)
            : ((remainder == 0U)
                ? all_ones()
                : static_cast<word_type>((static_cast<word_type>(1U) << remainder) - static_cast<word_type>(1U)));
    }

    /**
     * @brief Clears any unused bits in the last storage word to maintain consistency with the bitset size.
     * @note This function should be called after any operation that may modify the last storage word to ensure that unused bits are cleared.
     */
    void trim_unused_bits() CASTLE_NOEXCEPT
    {
        if (word_count != 0U)
        {
            words_[word_count - 1U] &= top_mask_value();
        }
    }

    /**
     * @brief Computes the index of the storage word that contains the specified bit position.
     * @param position The bit position for which to compute the storage word index.
     * @return The index of the storage word containing the specified bit position.
     */
    static size_type word_index(size_type position) CASTLE_NOEXCEPT
    {
        return position / word_bits;
    }

    /**
     * @brief Computes the bit mask for the specified bit position within its storage word.
     * @param position The bit position for which to compute the bit mask.
     * @return The bit mask corresponding to the specified bit position within its storage word.
     * @note The returned mask has only the bit corresponding to the specified position set, all other bits are cleared.
     */
    static word_type bit_mask(size_type position) CASTLE_NOEXCEPT
    {
        return static_cast<word_type>(static_cast<word_type>(1U) << (position % word_bits));
    }

    /**
     * @brief Counts the number of set bits (population count) in the given storage word.
     * @param value The storage word whose set bits are to be counted.
     * @return The number of set bits in the given storage word.
     * @note This function uses compiler intrinsics for GCC/Clang if available, otherwise it falls back to a manual bit counting loop.
     */
    static size_type popcount_word(word_type value) CASTLE_NOEXCEPT
    {
#if defined(__GNUC__) || defined(__clang__)
        CASTLE_IF_CONSTEXPR (sizeof(word_type) <= sizeof(unsigned int))
        {
            return static_cast<size_type>(__builtin_popcount(static_cast<unsigned int>(value)));
        }
        else CASTLE_IF_CONSTEXPR (sizeof(word_type) <= sizeof(unsigned long))
        {
            return static_cast<size_type>(__builtin_popcountl(static_cast<unsigned long>(value)));
        }
        else
        {
            return static_cast<size_type>(__builtin_popcountll(static_cast<unsigned long long>(value)));
        }
#else
        size_type result = 0U;
        word_type remaining = value;
        while (remaining != static_cast<word_type>(0))
        {
            remaining &= static_cast<word_type>(remaining - static_cast<word_type>(1U));
            ++result;
        }
        return result;
#endif
    }

public:

    /**
     * @brief Proxy object returned by writable `operator[]`.
     *
     * It refers back to the owning bitset and updates the selected bit in
     * place without allocating storage.
     */
    class reference
    {
    public:
        /**
         * @brief Creates a reference proxy for one bit position.
         * @param owner Bitset that owns the bit.
         * @param position Bit position inside the bitset.
         */
        reference(bitset& owner, size_type position) CASTLE_NOEXCEPT
            : owner_(&owner)
            , position_(position)
        {
        }

        /** @brief Assigns a boolean value to the referenced bit. */
        reference& operator=(bool value) CASTLE_NOEXCEPT
        {
            owner_->set(position_, value);
            return *this;
        }

        /** @brief Assigns the value observed through another reference proxy. */
        reference& operator=(CASTLE_CONST reference& other) CASTLE_NOEXCEPT
        {
            owner_->set(position_, static_cast<bool>(other));
            return *this;
        }

        /** @brief Reads the referenced bit as a boolean value. */
        operator bool() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return owner_->test(position_);
        }

        /** @brief Returns the logical negation of the referenced bit. */
        bool operator~() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return !owner_->test(position_);
        }

        /** @brief Toggles the referenced bit and returns the proxy. */
        reference& flip() CASTLE_NOEXCEPT
        {
            owner_->flip(position_);
            return *this;
        }

    private:
        bitset* owner_;
        size_type position_;
    };

    /** @brief Constructs a bitset with every bit cleared. */
    CASTLE_CONSTEXPR bitset() CASTLE_NOEXCEPT
        : words_{0}
    {
    }

    /**
     * @brief Constructs a bitset from the low bits of an integer value.
     * @param value Source integer whose low-order bits initialize the bitset.
     */
    explicit bitset(unsigned long long value) CASTLE_NOEXCEPT
        : words_{0}
    {
        assign(value);
    }

    /**
     * @brief Constructs a bitset by parsing a null-terminated character sequence.
     * @param text Text containing zero and one characters.
     */
    explicit bitset(CASTLE_CONST char* text) CASTLE_NOEXCEPT
        : words_{0}
    {
        from_string(text);
    }

    bitset(CASTLE_CONST bitset&) CASTLE_DEFAULT;
    bitset(bitset&&) CASTLE_DEFAULT;

    bitset& operator=(CASTLE_CONST bitset&) CASTLE_DEFAULT;
    bitset& operator=(bitset&&) CASTLE_DEFAULT;

    /** @brief Returns the number of bits stored by the bitset. */
    static CASTLE_CONSTEXPR size_type size() CASTLE_NOEXCEPT
    {
        return Bits;
    }

    /** @brief Returns how many storage words are logically used. */
    static CASTLE_CONSTEXPR size_type number_of_words() CASTLE_NOEXCEPT
    {
        return word_count;
    }

    /** @brief Returns the number of bits in each storage word. */
    static CASTLE_CONSTEXPR size_type bits_per_word() CASTLE_NOEXCEPT
    {
        return word_bits;
    }

    /** @brief Returns a pointer to the underlying storage words. */
    word_type* data() CASTLE_NOEXCEPT
    {
        return words_;
    }

    /** @brief Returns a const pointer to the underlying storage words. */
    CASTLE_CONST word_type* data() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return words_;
    }

    /**
     * @brief Reports whether a specific bit is set.
     * @param position Bit index to inspect.
     * @return `true` when the selected bit is set; `false` for out-of-range positions.
     */
    bool test(size_type position) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (position >= Bits)
        {
            return false;
        }

        return (words_[word_index(position)] & bit_mask(position)) != static_cast<word_type>(0);
    }

    /** @brief Returns whether at least one bit is set. */
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

    /** @brief Returns whether all bits are clear. */
    CASTLE_NODISCARD bool none() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !any();
    }

    /** @brief Returns whether every bit in range is set. */
    CASTLE_NODISCARD bool all() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (Bits == 0U)
        {
            return true;
        }

        for (size_type i = 0U; i + 1U < word_count; ++i)
        {
            // LCOV_EXCL_START
            if (words_[i] != all_ones())
            {
                return false;
            }
            // LCOV_EXCL_STOP
        }

        return words_[word_count - 1U] == top_mask_value();
    }

    /** @brief Counts how many bits are currently set. */
    CASTLE_NODISCARD size_type count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type result = 0U;

        for (size_type i = 0U; i < word_count; ++i)
        {
            result += popcount_word(words_[i]);
        }

        return result;
    }

    /** @brief Returns the sentinel used when bit searches fail. */
    static CASTLE_CONSTEXPR size_type npos() CASTLE_NOEXCEPT
    {
        return Bits;
    }

    /** @brief Sets every bit in the bitset. */
    bitset& set() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = all_ones();
        }
        trim_unused_bits();
        return *this;
    }

    /**
     * @brief Sets or clears one bit.
     * @param position Bit index to modify.
     * @param value `true` to set the bit, `false` to clear it.
     * @return `*this`.
     */
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

    /** @brief Clears every bit in the bitset. */
    bitset& reset() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = static_cast<word_type>(0);
        }
        return *this;
    }

    /** @brief Clears one bit and returns `*this`. */
    bitset& reset(size_type position) CASTLE_NOEXCEPT
    {
        return set(position, false);
    }

    /** @brief Toggles every bit in the bitset. */
    bitset& flip() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] = static_cast<word_type>(~words_[i]);
        }
        trim_unused_bits();
        return *this;
    }

    /** @brief Toggles one bit and returns `*this`. */
    bitset& flip(size_type position) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (position >= Bits)
        {
            return *this;
        }
        // LCOV_EXCL_STOP

        words_[word_index(position)] ^= bit_mask(position);
        return *this;
    }

    /** @brief Replaces the contents with the low bits of `value`. */
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

    /**
     * @brief Parses bits from a character sequence.
     * @param text Source text to parse.
     * @param position Starting character offset inside `text`.
     * @param length Maximum number of characters to inspect.
     * @param zero Character interpreted as bit zero.
     * @param one Character interpreted as bit one.
     * @return `*this`.
     * @note Characters other than `zero` and `one` leave the corresponding bit clear.
     */
    bitset& from_string(CASTLE_CONST char* text,
                        size_type position = 0U,
                        size_type length = static_cast<size_type>(-1),
                        char zero = '0',
                        char one = '1') CASTLE_NOEXCEPT
    {
        reset();

        // LCOV_EXCL_START
        if (text == 0)
        {
            return *this;
        }
        // LCOV_EXCL_STOP

        size_type available = 0U;
        while (available < length && text[position + available] != '\0') // LCOV_EXCL_BR_LINE
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
                
            }
            else
            {
                
            }
        }

        return *this;
    }

    /** @brief Packs the bitset into an `unsigned long`. */
    unsigned long to_ulong() CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(Bits <= sizeof(unsigned long) * CHAR_BIT,
                      "castle::bitset::to_ulong() target type is too small");
        
        
        unsigned long result = 0UL;
        for (size_type i = 0U; i < word_count; ++i)
        {
            result |= static_cast<unsigned long>(words_[i]) << (i * word_bits); // Packs whole storage words directly into the result value.
        }
        return result;
    }

    /** @brief Packs the bitset into an `unsigned long long`. */
    unsigned long long to_ullong() CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(Bits <= sizeof(unsigned long long) * CHAR_BIT,
                      "castle::bitset::to_ullong() target type is too small");
        
        
        unsigned long long result = 0ULL;
        for (size_type i = 0U; i < word_count; ++i)
        {
            result |= static_cast<unsigned long long>(words_[i]) << (i * word_bits); // Packs whole storage words directly into the result value.
        }
        return result;
    }

    /**
     * @brief Writes the bitset into a caller-owned character buffer.
     * @param buffer Destination buffer.
     * @param capacity Size of `buffer` in characters.
     * @param zero Character used for cleared bits.
     * @param one Character used for set bits.
     * @return `true` on success, or `false` when the buffer is null or too small.
     */
    bool to_string(char* buffer,
                   size_type capacity,
                   char zero = '0',
                   char one = '1') CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (buffer == 0 || capacity < Bits + 1U) // LCOV_EXCL_BR_LINE
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

    /** @brief Returns the first bit index matching `state`, or `npos()`. */
    size_type find_first(bool state) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Bits; ++i) // LCOV_EXCL_BR_LINE
        {
            if (test(i) == state)
            {
                return i;
            }
        }
        return npos(); // LCOV_EXCL_LINE
    }

    /** @brief Returns the next bit index after `position` matching `state`, or `npos()`. */
    size_type find_next(size_type position, bool state) CASTLE_CONST CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (position >= Bits)
        {
            return npos();
        }
        // LCOV_EXCL_STOP

        for (size_type i = position + 1U; i < Bits; ++i)
        {
            if (test(i) == state)
            {
                return i;
            }
        }
        return npos();
    }

    /** @brief Returns a writable proxy for one bit. */
    reference operator[](size_type position) CASTLE_NOEXCEPT
    {
        return reference(*this, position);
    }

    /** @brief Returns the current value of one bit. */
    bool operator[](size_type position) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return test(position);
    }

    /** @brief Applies a bitwise AND with `other` in place. */
    bitset& operator&=(CASTLE_CONST bitset& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] &= other.words_[i];
        }
        return *this;
    }

    /** @brief Applies a bitwise OR with `other` in place. */
    bitset& operator|=(CASTLE_CONST bitset& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] |= other.words_[i];
        }
        return *this;
    }

    /** @brief Applies a bitwise XOR with `other` in place. */
    bitset& operator^=(CASTLE_CONST bitset& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < word_count; ++i)
        {
            words_[i] ^= other.words_[i];
        }
        trim_unused_bits();
        return *this;
    }

    /** @brief Shifts all bits left by `shift` positions in place. */
    bitset& operator<<=(size_type shift) CASTLE_NOEXCEPT
    {
        if (shift >= Bits)
        {
            return reset();
        }

        // LCOV_EXCL_START
        if (shift == 0U || word_count == 0U)
        {
            return *this;
        }
        // LCOV_EXCL_STOP

        CASTLE_CONST size_type word_shift = shift / word_bits;
        CASTLE_CONST size_type bit_shift = shift % word_bits;

        for (size_type i = word_count; i > 0U; --i)
        {
            CASTLE_CONST size_type destination = i - 1U;
            word_type value = static_cast<word_type>(0);

            if (destination >= word_shift) // LCOV_EXCL_BR_LINE
            {
                CASTLE_CONST size_type source = destination - word_shift;
                value = static_cast<word_type>(words_[source] << bit_shift);

                // LCOV_EXCL_START
                if (bit_shift != 0U && source > 0U)
                {
                    value |= static_cast<word_type>(words_[source - 1U] >> (word_bits - bit_shift));
                }
                // LCOV_EXCL_STOP
            }

            words_[destination] = value;
        }

        trim_unused_bits();
        return *this;
    }

    /** @brief Shifts all bits right by `shift` positions in place. */
    bitset& operator>>=(size_type shift) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (shift >= Bits)
        {
            return reset();
        }

        if (shift == 0U || word_count == 0U)
        {
            return *this;
        }
        // LCOV_EXCL_STOP

        CASTLE_CONST size_type word_shift = shift / word_bits;
        CASTLE_CONST size_type bit_shift = shift % word_bits;

        for (size_type destination = 0U; destination < word_count; ++destination)
        {
            word_type value = static_cast<word_type>(0);
            CASTLE_CONST size_type source = destination + word_shift;

            // LCOV_EXCL_START
            if (source < word_count)
            {
                value = static_cast<word_type>(words_[source] >> bit_shift);

                if (bit_shift != 0U && source + 1U < word_count)
                {
                    value |= static_cast<word_type>(words_[source + 1U] << (word_bits - bit_shift));
                }
            }
            // LCOV_EXCL_STOP

            words_[destination] = value;
        }

        trim_unused_bits();
        return *this;
    }

    /** @brief Exchanges the storage words of two bitsets. */
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

/** @brief Returns the bitwise AND of two bitsets. */
template <size_type Bits, typename Word>
bitset<Bits, Word> operator&(bitset<Bits, Word> lhs,
                             CASTLE_CONST bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    lhs &= rhs;
    return lhs;
}

/** @brief Returns the bitwise OR of two bitsets. */
template <size_type Bits, typename Word>
bitset<Bits, Word> operator|(bitset<Bits, Word> lhs,
                             CASTLE_CONST bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    lhs |= rhs;
    return lhs;
}

/** @brief Returns the bitwise XOR of two bitsets. */
template <size_type Bits, typename Word>
bitset<Bits, Word> operator^(bitset<Bits, Word> lhs,
                             CASTLE_CONST bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    lhs ^= rhs;
    return lhs;
}

/** @brief Returns a copy of `lhs` shifted left by `shift` bits. */
template <size_type Bits, typename Word>
bitset<Bits, Word> operator<<(bitset<Bits, Word> lhs,
                              size_type shift) CASTLE_NOEXCEPT
{
    lhs <<= shift;
    return lhs;
}

/** @brief Returns a copy of `lhs` shifted right by `shift` bits. */
template <size_type Bits, typename Word>
bitset<Bits, Word> operator>>(bitset<Bits, Word> lhs,
                              size_type shift) CASTLE_NOEXCEPT
{
    lhs >>= shift;
    return lhs;
}

/** @brief Returns a copy of `value` with every bit toggled. */
template <size_type Bits, typename Word>
bitset<Bits, Word> operator~(CASTLE_CONST bitset<Bits, Word>& value) CASTLE_NOEXCEPT
{
    bitset<Bits, Word> result(value);
    result.flip();
    return result;
}

/** @brief Returns whether two bitsets have identical storage contents. */
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

/** @brief Returns whether two bitsets differ. */
template <size_type Bits, typename Word>
bool operator!=(CASTLE_CONST bitset<Bits, Word>& lhs,
                CASTLE_CONST bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/** @brief Swaps two bitsets. */
template <size_type Bits, typename Word>
void swap(bitset<Bits, Word>& lhs,
          bitset<Bits, Word>& rhs) CASTLE_NOEXCEPT
{
    lhs.swap(rhs);
}

} 

#endif 
