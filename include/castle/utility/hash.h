#ifndef CASTLE_UTILITY_HASH_H
#define CASTLE_UTILITY_HASH_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/utility/compare.h"

#include "castle/container/string_view.h"

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

template <typename T, bool IsIntegral = meta::is_integral<T>::value, bool IsEnum = meta::is_enum<T>::value, bool IsPointer = meta::is_pointer<T>::value>
struct hash_impl;

template <typename T>
struct hash_impl<T, true, false, false>
{
    size_type operator()(CASTLE_CONST T& value) const CASTLE_NOEXCEPT
    {
        return detail::hash_integral(value);
    }
};

template <typename T>
struct hash_impl<T, false, true, false>
{
    size_type operator()(CASTLE_CONST T& value) const CASTLE_NOEXCEPT
    {
        return detail::hash_integral(value);
    }
};

template <typename T>
struct hash_impl<T, false, false, true>
{
    size_type operator()(CASTLE_CONST T& value) const CASTLE_NOEXCEPT
    {
        return static_cast<size_type>(detail::hash_mix64(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(value))));
    }
};

template <typename T>
struct hash
{
    size_type operator()(CASTLE_CONST T& value) const CASTLE_NOEXCEPT
    {
        static_assert(meta::is_integral<T>::value || meta::is_enum<T>::value || meta::is_pointer<T>::value,
                      "castle::hash<T>: provide a hash specialization for this type");
        return hash_impl<T>()(value);
    }
};

template <typename T>
struct hash<CASTLE_CONST T>
{
    size_type operator()(CASTLE_CONST T& value) const CASTLE_NOEXCEPT
    {
        return hash<T>()(value);
    }
};

template <typename CharT>
struct hash<container::basic_string_view<CharT>>
{
    size_type operator()(container::basic_string_view<CharT> value) const CASTLE_NOEXCEPT
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

#endif // CASTLE_UTILITY_HASH_H
