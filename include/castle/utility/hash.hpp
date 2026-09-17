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

#include "castle/container/string_view.hpp"

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
struct hash<container::basic_string_view<CharT>>
{
    /**
     * @brief Hashes every byte of the view using an FNV-style stream and final mix.
     * @param value String view to hash.
     * @return Deterministic hash value.
     */
    size_type operator()(container::basic_string_view<CharT> value) CASTLE_CONST CASTLE_NOEXCEPT
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
