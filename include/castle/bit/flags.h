#ifndef CASTLE_BIT_FLAGS_H
#define CASTLE_BIT_FLAGS_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/utility/move.h"

#include <limits.h> // CHAR_BIT

namespace castle
{
namespace bit
{

// ============================================================================
// flags
// ----------------------------------------------------------------------------
// A small, type-safe wrapper around an unsigned integral flag value.
//
// MASK defines which bits belong to the flag set. Every operation that can
// introduce bits applies MASK, keeping the stored value deterministic.
//
// This is intentionally value based. No dynamic storage, virtual dispatch,
// RTTI, exceptions, or STL facilities are required.
// ============================================================================
template <typename T, T MASK = static_cast<T>(~T(0))>
class flags
{
public:
    static_assert(meta::is_valid_integer<T>::value && meta::is_unsigned<T>::value,
                  "castle::bit::flags requires an unsigned integral type");

    using value_type = T;

    static CASTLE_CONSTEXPR value_type ALL_SET = static_cast<value_type>(~value_type(0)) & MASK;
    static CASTLE_CONSTEXPR value_type ALL_CLEAR = value_type(0);
    static CASTLE_CONSTEXPR size_t NBITS = sizeof(value_type) * CHAR_BIT;

    // ------------------------------------------------------------------------
    // Constructors.
    // ------------------------------------------------------------------------
    CASTLE_CONSTEXPR flags() CASTLE_NOEXCEPT
        : data_(ALL_CLEAR)
    {
    }

    CASTLE_CONSTEXPR flags(value_type pattern) CASTLE_NOEXCEPT
        : data_(static_cast<value_type>(pattern & MASK))
    {
    }

    CASTLE_CONSTEXPR flags(CASTLE_CONST flags& other) CASTLE_NOEXCEPT
        : data_(other.data_)
    {
    }

    CASTLE_CONSTEXPR flags(flags&& other) CASTLE_NOEXCEPT
        : data_(castle::move(other.data_))
    {
    }

    // ------------------------------------------------------------------------
    // Tests bits.
    // ------------------------------------------------------------------------
    template <value_type pattern>
    CASTLE_CONSTEXPR bool test() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & pattern) != ALL_CLEAR;
    }

    CASTLE_CONSTEXPR bool test(value_type pattern) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & pattern) != ALL_CLEAR;
    }

    // ------------------------------------------------------------------------
    // Set bits.
    // ------------------------------------------------------------------------
    template <value_type pattern, bool value>
    CASTLE_CONSTEXPR flags& set() CASTLE_NOEXCEPT
    {
        return set(pattern, value);
    }

    template <value_type pattern>
    CASTLE_CONSTEXPR flags& set(bool value) CASTLE_NOEXCEPT
    {
        return set(pattern, value);
    }

    template <value_type pattern>
    CASTLE_CONSTEXPR flags& set() CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ | (pattern & MASK));
        return *this;
    }

    CASTLE_CONSTEXPR flags& set(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ | (pattern & MASK));
        return *this;
    }

    CASTLE_CONSTEXPR flags& set(value_type pattern, bool value) CASTLE_NOEXCEPT
    {
        if (value)
        {
            data_ = static_cast<value_type>(data_ | (pattern & MASK));
        }
        else
        {
            data_ = static_cast<value_type>(data_ & static_cast<value_type>(~pattern & MASK));
        }

        return *this;
    }

    // ------------------------------------------------------------------------
    // Clear all bits.
    // ------------------------------------------------------------------------
    flags& clear() CASTLE_NOEXCEPT
    {
        data_ = ALL_CLEAR;
        return *this;
    }

    // ------------------------------------------------------------------------
    // Reset bits selected by pattern.
    // ------------------------------------------------------------------------
    template <value_type pattern>
    flags& reset() CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ & static_cast<value_type>(~pattern));
        return *this;
    }

    flags& reset(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ & static_cast<value_type>(~pattern));
        return *this;
    }

    // ------------------------------------------------------------------------
    // Flip bits.
    // ------------------------------------------------------------------------
    flags& flip() CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(~data_ & MASK);
        return *this;
    }

    template <value_type pattern>
    flags& flip() CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ ^ (pattern & MASK));
        return *this;
    }

    flags& flip(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ ^ (pattern & MASK));
        return *this;
    }

    // ------------------------------------------------------------------------
    // Query the complete flag set.
    // ------------------------------------------------------------------------
    CASTLE_CONSTEXPR bool all() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_ == ALL_SET;
    }

    template <value_type pattern>
    CASTLE_CONSTEXPR bool all_of() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & static_cast<value_type>(pattern & MASK)) ==
               static_cast<value_type>(pattern & MASK);
    }

    CASTLE_CONSTEXPR bool all_of(value_type pattern) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST value_type masked_pattern = static_cast<value_type>(pattern & MASK);
        return (data_ & masked_pattern) == masked_pattern;
    }

    // ------------------------------------------------------------------------
    // Query that no selected bits are set.
    // ------------------------------------------------------------------------
    CASTLE_CONSTEXPR bool none() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_ == ALL_CLEAR;
    }

    template <value_type pattern>
    CASTLE_CONSTEXPR bool none_of() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & static_cast<value_type>(pattern & MASK)) == ALL_CLEAR;
    }

    CASTLE_CONSTEXPR bool none_of(value_type pattern) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !any_of(pattern);
    }

    // ------------------------------------------------------------------------
    // Query that at least one selected bit is set.
    // ------------------------------------------------------------------------
    CASTLE_CONSTEXPR bool any() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_ != ALL_CLEAR;
    }

    template <value_type pattern>
    CASTLE_CONSTEXPR bool any_of() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & static_cast<value_type>(pattern & MASK)) != ALL_CLEAR;
    }

    CASTLE_CONSTEXPR bool any_of(value_type pattern) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & static_cast<value_type>(pattern & MASK)) != ALL_CLEAR;
    }

    // ------------------------------------------------------------------------
    // Return or replace the complete stored value.
    // ------------------------------------------------------------------------
    CASTLE_CONSTEXPR value_type value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_;
    }

    flags& value(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(pattern & MASK);
        return *this;
    }

    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_;
    }

    // ------------------------------------------------------------------------
    // Compound bitwise operators.
    // ------------------------------------------------------------------------
    flags& operator&=(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ & pattern);
        return *this;
    }

    flags& operator|=(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ | (pattern & MASK));
        return *this;
    }

    flags& operator^=(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ ^ (pattern & MASK));
        return *this;
    }

    // ------------------------------------------------------------------------
    // Assignment.
    // ------------------------------------------------------------------------
    flags& operator=(CASTLE_CONST flags& other) CASTLE_NOEXCEPT
    {
        data_ = other.data_;
        return *this;
    }

    flags& operator=(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(pattern & MASK);
        return *this;
    }

    // ------------------------------------------------------------------------
    // Swap.
    // ------------------------------------------------------------------------
    void swap(flags& other) CASTLE_NOEXCEPT
    {
        CASTLE_CONST value_type temporary = data_;
        data_ = other.data_;
        other.data_ = temporary;
    }

private:
    value_type data_;
};

template <typename T, T MASK>
CASTLE_CONSTEXPR typename flags<T, MASK>::value_type flags<T, MASK>::ALL_SET;

template <typename T, T MASK>
CASTLE_CONSTEXPR typename flags<T, MASK>::value_type flags<T, MASK>::ALL_CLEAR;

template <typename T, T MASK>
CASTLE_CONSTEXPR size_t flags<T, MASK>::NBITS;

// ============================================================================
// Relational operators.
// ============================================================================
template <typename T, T MASK>
CASTLE_CONSTEXPR bool operator==(CASTLE_CONST flags<T, MASK>& lhs,
                                CASTLE_CONST flags<T, MASK>& rhs) CASTLE_NOEXCEPT
{
    return lhs.value() == rhs.value();
}

template <typename T, T MASK>
CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST flags<T, MASK>& lhs,
                                CASTLE_CONST flags<T, MASK>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

// ============================================================================
// Free swap.
// ============================================================================
template <typename T, T MASK>
void swap(flags<T, MASK>& lhs, flags<T, MASK>& rhs) CASTLE_NOEXCEPT
{
    lhs.swap(rhs);
}

} // namespace bit
} // namespace castle

#endif // CASTLE_BIT_FLAGS_H
