// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file hash.hpp
 * @brief Deterministic hash functors for integral, enum, pointer, and string-view keys.
 *
 * Use this header for fixed-capacity hash tables or registries that need a
 * simple, allocation-free hashing primitive without the standard library. The
 * default `castle::hash<T>` supports integral types, enums, pointers, and
 * Castle string views. Custom key types require a specialization.
 *
 * @code
 * castle::size_type key = castle::hash<uint32_t>()(42U);
 * @endcode
 */
#ifndef CASTLE_UTILITY_HASH_HPP
#define CASTLE_UTILITY_HASH_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/compare.hpp"

#include "castle/dsa/string_view.hpp"

#include <stdint.h>

namespace castle
{

namespace detail
{

static uint64_t hash_mix64(uint64_t value) CASTLE_NOEXCEPT
{
    value ^= value >> 30U;
    value *= 0xBF58476D1CE4E5BULL;
    value ^= value >> 27U;
    value *= 0x94D049BB133111EBULL;
    value ^= value >> 31U;
    return value;
}

template <typename T>
static size_type hash_integral(CASTLE_CONST T& value) CASTLE_NOEXCEPT
{
    return static_cast<size_type>(hash_mix64(static_cast<uint64_t>(value)));
}

/**
 * @brief Enhanced double hashing index sequence over `[0, Range)` (Dillinger and Manolios).
 * @tparam Range Number of slots. Must be in `1 .. size_type(-1) / 2`.
 * @note Built from one hash value `h1`: `x = h1 mod Range`, `y = hash_mix64(h1) mod Range`, then
 * `x += y; y += i` for each next index. Both terms stay below `Range`, so each step needs one
 * compare-and-subtract and no division. A power-of-two `Range` turns both initial reductions into
 * masks. Shared by `dsa::bloom_filter` (bit positions) and `dsa::count_min_sketch` (row columns).
 * @warning Call `advance()` at most `Range` times; later steps could leave `[0, Range)`.
 */
template <size_type Range>
class double_hash_probe
{
    static_assert(Range > 0U, "detail::double_hash_probe requires a positive range");
    static_assert(Range <= static_cast<size_type>(-1) / 2U, "detail::double_hash_probe range is too large");

public:
    explicit double_hash_probe(size_type hash_value) CASTLE_NOEXCEPT
        : index_(reduce(hash_value))
        , step_(reduce(static_cast<size_type>(hash_mix64(static_cast<uint64_t>(hash_value)))))
        , round_(0U)
    {
    }

    /** @brief Returns the current index, `0 .. Range - 1`. */
    size_type index() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index_;
    }

    /** @brief Moves to the next index of the sequence. */
    void advance() CASTLE_NOEXCEPT
    {
        ++round_;
        index_ = add_mod(index_, step_);
        step_ = add_mod(step_, round_);
    }

private:
    static size_type reduce(size_type value) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (meta::is_power_of_two<Range>::value)
        {
            return value & (Range - 1U);
        }
        else
        {
            return value % Range;
        }
    }

    /** @brief Returns `(lhs + rhs) mod Range` for `lhs < Range` and `rhs <= Range`. */
    static size_type add_mod(size_type lhs, size_type rhs) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type sum = lhs + rhs;
        return (sum >= Range) ? (sum - Range) : sum;
    }

    size_type index_;
    size_type step_;
    size_type round_;
};

} // namespace detail

/**
 * @brief Helper used by `castle::hash<T>` to select the built-in hashing path.
 * @tparam T Key type.
 * @tparam IsIntegral Whether `T` is integral.
 * @tparam IsEnum Whether `T` is an enumeration.
 * @tparam IsPointer Whether `T` is a pointer.
 */
template <typename T, bool IsIntegral = meta::is_integral<T>::value, bool IsEnum = meta::is_enum<T>::value, bool IsPointer = meta::is_pointer<T>::value>
struct hash_impl;

/** @brief Hash implementation for integral keys. */
template <typename T>
struct hash_impl<T, true, false, false>
{
    /** @brief Hashes an integral value. */
    size_type operator()(CASTLE_CONST T& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return detail::hash_integral(value);
    }
};

/** @brief Hash implementation for enumeration keys. */
template <typename T>
struct hash_impl<T, false, true, false>
{
    /** @brief Hashes an enumeration value through its underlying bits. */
    size_type operator()(CASTLE_CONST T& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return detail::hash_integral(value);
    }
};

/** @brief Hash implementation for pointer keys. */
template <typename T>
struct hash_impl<T, false, false, true>
{
    /** @brief Hashes a pointer value by mixing its address bits. */
    size_type operator()(CASTLE_CONST T& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<size_type>(detail::hash_mix64(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(value))));
    }
};

/**
 * @brief Default hash functor for Castle-supported key types.
 * @tparam T Key type to hash.
 */
template <typename T>
struct hash
{
    /**
     * @brief Hashes a supported key value.
     * @param value Key to hash.
     * @return Deterministic hash value.
     * @warning Unsupported user-defined types require a specialization.
     */
    size_type operator()(CASTLE_CONST T& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(meta::is_integral<T>::value || meta::is_enum<T>::value || meta::is_pointer<T>::value,
                      "castle::hash<T>: provide a hash specialization for this type");
        return hash_impl<T>()(value);
    }
};

/**
 * @brief `const`-qualified forwarding specialization of `castle::hash<T>`.
 * @tparam T Underlying key type.
 */
template <typename T>
struct hash<CASTLE_CONST T>
{
    /** @brief Hashes a `const` value by delegating to `castle::hash<T>`. */
    size_type operator()(CASTLE_CONST T& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hash<T>()(value);
    }
};

/**
 * @brief Hash specialization for Castle string views.
 * @tparam CharT Character type stored by the view.
 */
template <typename CharT>
struct hash<dsa::basic_string_view<CharT>>
{
    /**
     * @brief Hashes every byte of the view using an FNV-style stream and final mix.
     * @param value String view to hash.
     * @return Deterministic hash value.
     */
    size_type operator()(dsa::basic_string_view<CharT> value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        uint64_t state = 1469598103934665603ULL;
        for (size_type i = 0U; i < value.size(); ++i)
        {
            uint64_t unit = static_cast<uint64_t>(value[i]);
            for (unsigned int shift = 0U; shift < sizeof(CharT) * 8U; shift += 8U)
            {
                state ^= (unit >> shift) & 0xFFU;
                state *= 1099511628211ULL;
            }
        }
        return static_cast<size_type>(detail::hash_mix64(state));
    }
};

} // namespace castle

#endif // CASTLE_UTILITY_HASH_HPP
