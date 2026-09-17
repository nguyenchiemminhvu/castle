// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file flags.hpp
 * @brief Small masked flag-set wrapper for unsigned integral values.
 *
 * Use this header when code needs a deterministic, type-safe wrapper around a
 * register shadow, feature bitset, or other packed flag word while constraining
 * all stored bits to a compile-time mask.
 *
 * Key constraints:
 * - `T` must be an unsigned Castle integer type.
 * - Operations that can introduce bits apply `MASK`, preserving the class invariant.
 * - No allocation, exceptions, RTTI, virtual dispatch, or STL facilities are used.
 */
#ifndef CASTLE_BIT_FLAGS_HPP
#define CASTLE_BIT_FLAGS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/move.hpp"

#include <limits.h>

namespace castle
{
namespace bit
{

/**
 * @brief Value-based wrapper for a masked set of unsigned flags.
 * @tparam T Unsigned integral storage type.
 * @tparam MASK Compile-time mask that defines which bits may be stored.
 */
template <typename T, T MASK = static_cast<T>(~T(0))>
class flags
{
public:
    static_assert(meta::is_valid_integer<T>::value && meta::is_unsigned<T>::value,
                  "castle::bit::flags requires an unsigned integral type");

    using value_type = T;

    /** @brief Value whose mask-selected bits are all set. */
    static CASTLE_CONSTEXPR value_type ALL_SET = static_cast<value_type>(~value_type(0)) & MASK;
    /** @brief Value whose bits are all clear. */
    static CASTLE_CONSTEXPR value_type ALL_CLEAR = value_type(0);
    /** @brief Storage width of `value_type` in bits. */
    static CASTLE_CONSTEXPR size_t NBITS = sizeof(value_type) * CHAR_BIT;

    /**
     * @brief Constructs an empty flag set.
     */
    CASTLE_CONSTEXPR flags() CASTLE_NOEXCEPT
        : data_(ALL_CLEAR)
    {
    }

    /**
     * @brief Constructs a flag set from a raw pattern.
     * @param pattern Initial flag pattern.
     * @note Bits outside `MASK` are discarded.
     */
    CASTLE_CONSTEXPR flags(value_type pattern) CASTLE_NOEXCEPT
        : data_(static_cast<value_type>(pattern & MASK))
    {
    }

    /**
     * @brief Copy-constructs a flag set.
     * @param other Source flag set.
     */
    CASTLE_CONSTEXPR flags(CASTLE_CONST flags& other) CASTLE_NOEXCEPT
        : data_(other.data_)
    {
    }

    /**
     * @brief Move-constructs a flag set.
     * @param other Source flag set.
     */
    CASTLE_CONSTEXPR flags(flags&& other) CASTLE_NOEXCEPT
        : data_(castle::move(other.data_))
    {
    }

    /**
     * @brief Tests whether any bit from a compile-time pattern is set.
     * @tparam pattern Pattern to test.
     * @return `true` when at least one selected bit is set; otherwise `false`.
     */
    template <value_type pattern>
    CASTLE_CONSTEXPR bool test() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & pattern) != ALL_CLEAR;
    }

    /**
     * @brief Tests whether any bit from a runtime pattern is set.
     * @param pattern Pattern to test.
     * @return `true` when at least one selected bit is set; otherwise `false`.
     */
    CASTLE_CONSTEXPR bool test(value_type pattern) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & pattern) != ALL_CLEAR;
    }

    /**
     * @brief Sets or clears a compile-time pattern according to a compile-time boolean.
     * @tparam pattern Pattern to modify.
     * @tparam value `true` to set the pattern, `false` to clear it.
     * @return `*this`.
     */
    template <value_type pattern, bool value>
    CASTLE_CONSTEXPR flags& set() CASTLE_NOEXCEPT
    {
        return set(pattern, value);
    }

    /**
     * @brief Sets or clears a compile-time pattern according to a runtime boolean.
     * @tparam pattern Pattern to modify.
     * @param value `true` to set the pattern, `false` to clear it.
     * @return `*this`.
     */
    template <value_type pattern>
    CASTLE_CONSTEXPR flags& set(bool value) CASTLE_NOEXCEPT
    {
        return set(pattern, value);
    }

    /**
     * @brief Sets a compile-time pattern.
     * @tparam pattern Pattern to set.
     * @return `*this`.
     * @note Bits outside `MASK` are discarded before the update.
     */
    template <value_type pattern>
    CASTLE_CONSTEXPR flags& set() CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ | (pattern & MASK));
        return *this;
    }

    /**
     * @brief Sets a runtime pattern.
     * @param pattern Pattern to set.
     * @return `*this`.
     * @note Bits outside `MASK` are discarded before the update.
     */
    CASTLE_CONSTEXPR flags& set(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ | (pattern & MASK));
        return *this;
    }

    /**
     * @brief Sets or clears a runtime pattern according to a runtime boolean.
     * @param pattern Pattern to modify.
     * @param value `true` to set the pattern, `false` to clear it.
     * @return `*this`.
     * @note Bits outside `MASK` are discarded before the update.
     */
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

    /**
     * @brief Clears every stored bit.
     * @return `*this`.
     */
    flags& clear() CASTLE_NOEXCEPT
    {
        data_ = ALL_CLEAR;
        return *this;
    }

    /**
     * @brief Clears every bit selected by a compile-time pattern.
     * @tparam pattern Pattern to clear.
     * @return `*this`.
     */
    template <value_type pattern>
    flags& reset() CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ & static_cast<value_type>(~pattern));
        return *this;
    }

    /**
     * @brief Clears every bit selected by a runtime pattern.
     * @param pattern Pattern to clear.
     * @return `*this`.
     */
    flags& reset(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ & static_cast<value_type>(~pattern));
        return *this;
    }

    /**
     * @brief Inverts every mask-selected bit.
     * @return `*this`.
     */
    flags& flip() CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(~data_ & MASK);
        return *this;
    }

    /**
     * @brief Inverts every bit selected by a compile-time pattern.
     * @tparam pattern Pattern to toggle.
     * @return `*this`.
     */
    template <value_type pattern>
    flags& flip() CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ ^ (pattern & MASK));
        return *this;
    }

    /**
     * @brief Inverts every bit selected by a runtime pattern.
     * @param pattern Pattern to toggle.
     * @return `*this`.
     */
    flags& flip(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ ^ (pattern & MASK));
        return *this;
    }

    /**
     * @brief Tests whether every mask-selected bit is set.
     * @return `true` when `data_ == ALL_SET`; otherwise `false`.
     */
    CASTLE_CONSTEXPR bool all() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_ == ALL_SET;
    }

    /**
     * @brief Tests whether every bit from a compile-time pattern is set.
     * @tparam pattern Pattern to test.
     * @return `true` when all mask-selected bits from `pattern` are present.
     */
    template <value_type pattern>
    CASTLE_CONSTEXPR bool all_of() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & static_cast<value_type>(pattern & MASK)) ==
               static_cast<value_type>(pattern & MASK);
    }

    /**
     * @brief Tests whether every bit from a runtime pattern is set.
     * @param pattern Pattern to test.
     * @return `true` when all mask-selected bits from `pattern` are present.
     */
    CASTLE_CONSTEXPR bool all_of(value_type pattern) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST value_type masked_pattern = static_cast<value_type>(pattern & MASK);
        return (data_ & masked_pattern) == masked_pattern;
    }

    /**
     * @brief Tests whether no bits are set.
     * @return `true` when `data_ == ALL_CLEAR`; otherwise `false`.
     */
    CASTLE_CONSTEXPR bool none() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_ == ALL_CLEAR;
    }

    /**
     * @brief Tests whether no bits from a compile-time pattern are set.
     * @tparam pattern Pattern to test.
     * @return `true` when every mask-selected bit from `pattern` is clear.
     */
    template <value_type pattern>
    CASTLE_CONSTEXPR bool none_of() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & static_cast<value_type>(pattern & MASK)) == ALL_CLEAR;
    }

    /**
     * @brief Tests whether no bits from a runtime pattern are set.
     * @param pattern Pattern to test.
     * @return `true` when every mask-selected bit from `pattern` is clear.
     */
    CASTLE_CONSTEXPR bool none_of(value_type pattern) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !any_of(pattern);
    }

    /**
     * @brief Tests whether any bit is set.
     * @return `true` when at least one bit is set; otherwise `false`.
     */
    CASTLE_CONSTEXPR bool any() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_ != ALL_CLEAR;
    }

    /**
     * @brief Tests whether any bit from a compile-time pattern is set.
     * @tparam pattern Pattern to test.
     * @return `true` when at least one mask-selected bit from `pattern` is set.
     */
    template <value_type pattern>
    CASTLE_CONSTEXPR bool any_of() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & static_cast<value_type>(pattern & MASK)) != ALL_CLEAR;
    }

    /**
     * @brief Tests whether any bit from a runtime pattern is set.
     * @param pattern Pattern to test.
     * @return `true` when at least one mask-selected bit from `pattern` is set.
     */
    CASTLE_CONSTEXPR bool any_of(value_type pattern) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (data_ & static_cast<value_type>(pattern & MASK)) != ALL_CLEAR;
    }

    /**
     * @brief Returns the stored flag pattern.
     * @return Current masked value.
     */
    CASTLE_CONSTEXPR value_type value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_;
    }

    /**
     * @brief Replaces the stored flag pattern.
     * @param pattern New flag pattern.
     * @return `*this`.
     * @note Bits outside `MASK` are discarded.
     */
    flags& value(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(pattern & MASK);
        return *this;
    }

    /**
     * @brief Converts the wrapper to the underlying value type.
     * @return Current masked value.
     */
    CASTLE_CONSTEXPR operator value_type() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_;
    }

    /**
     * @brief Applies a bitwise AND assignment with a raw pattern.
     * @param pattern Pattern to apply.
     * @return `*this`.
     */
    flags& operator&=(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ & pattern);
        return *this;
    }

    /**
     * @brief Applies a bitwise OR assignment with a raw pattern.
     * @param pattern Pattern to apply.
     * @return `*this`.
     * @note Bits outside `MASK` are discarded from `pattern` before the update.
     */
    flags& operator|=(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ | (pattern & MASK));
        return *this;
    }

    /**
     * @brief Applies a bitwise XOR assignment with a raw pattern.
     * @param pattern Pattern to apply.
     * @return `*this`.
     * @note Bits outside `MASK` are discarded from `pattern` before the update.
     */
    flags& operator^=(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(data_ ^ (pattern & MASK));
        return *this;
    }

    /**
     * @brief Copy-assigns another flag set.
     * @param other Source flag set.
     * @return `*this`.
     */
    flags& operator=(CASTLE_CONST flags& other) CASTLE_NOEXCEPT
    {
        data_ = other.data_;
        return *this;
    }

    /**
     * @brief Assigns a raw flag pattern.
     * @param pattern New flag pattern.
     * @return `*this`.
     * @note Bits outside `MASK` are discarded.
     */
    flags& operator=(value_type pattern) CASTLE_NOEXCEPT
    {
        data_ = static_cast<value_type>(pattern & MASK);
        return *this;
    }

    /**
     * @brief Exchanges the stored values of two flag sets.
     * @param other Flag set to swap with.
     */
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

/**
 * @brief Compares two flag sets for equality.
 * @tparam T Unsigned integral storage type.
 * @tparam MASK Compile-time mask that constrains both operands.
 * @param lhs Left-hand operand.
 * @param rhs Right-hand operand.
 * @return `true` when both operands store the same masked value; otherwise `false`.
 */
template <typename T, T MASK>
CASTLE_CONSTEXPR bool operator==(CASTLE_CONST flags<T, MASK>& lhs,
                                CASTLE_CONST flags<T, MASK>& rhs) CASTLE_NOEXCEPT
{
    return lhs.value() == rhs.value();
}

/**
 * @brief Compares two flag sets for inequality.
 * @tparam T Unsigned integral storage type.
 * @tparam MASK Compile-time mask that constrains both operands.
 * @param lhs Left-hand operand.
 * @param rhs Right-hand operand.
 * @return `true` when the operands store different masked values; otherwise `false`.
 */
template <typename T, T MASK>
CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST flags<T, MASK>& lhs,
                                CASTLE_CONST flags<T, MASK>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Exchanges the stored values of two flag sets.
 * @tparam T Unsigned integral storage type.
 * @tparam MASK Compile-time mask that constrains both operands.
 * @param lhs First flag set.
 * @param rhs Second flag set.
 */
template <typename T, T MASK>
void swap(flags<T, MASK>& lhs, flags<T, MASK>& rhs) CASTLE_NOEXCEPT
{
    lhs.swap(rhs);
}

}
}

#endif
