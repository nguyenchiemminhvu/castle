#pragma once

#include <type_traits>

namespace castle
{
namespace types
{

// ──────────────────────────────────────────────────────────────────────────
// has_unique_types — check if all types in a parameter pack are unique.
// Returns true if all types are unique, false otherwise.
// ──────────────────────────────────────────────────────────────────────────
template <typename... Types>
struct has_unique_types;

template <>
struct has_unique_types<> : std::true_type {};

template <typename Head, typename... Tail>
struct has_unique_types<Head, Tail...>
{
    constexpr static bool value = (!std::is_same<Head, Tail>::value && ...) && has_unique_types<Tail...>::value;
};

template <typename T>
struct has_unique_types<T> : std::true_type {};

template <typename... Types>
inline constexpr bool has_unique_types_v = has_unique_types<Types...>::value;


// ──────────────────────────────────────────────────────────────────────────
// is_valid_integer — check if a type is a valid integer type
// (integral, not bool, and size is a power of 2).
// Returns true if the type is a valid integer, false otherwise.
// ──────────────────────────────────────────────────────────────────────────

template <typename T>
struct is_valid_integer
{
    static constexpr bool is_int = std::is_integral<T>::value && !std::is_same<T, bool>::value;
    static constexpr bool is_signed = std::is_signed<T>::value;
    static constexpr bool is_unsigned = std::is_unsigned<T>::value;
    static constexpr bool is_size_power_of_2 = (sizeof(T) > 0) && ((sizeof(T) & (sizeof(T) - 1)) == 0);

    static constexpr bool value = is_int && is_size_power_of_2;
};

template <typename T>
inline constexpr bool is_valid_integer_v = is_valid_integer<T>::value;

// ──────────────────────────────────────────────────────────────────────────
// whitespace — provides a compile-time list of whitespace characters for
// different character types.
// ──────────────────────────────────────────────────────────────────────────

template <typename TChar>
struct whitespace;

template <>
struct whitespace<char>
{
    static constexpr const char* value() noexcept { return " \t\n\r\f\v"; }
};

template <>
struct whitespace<wchar_t>
{
    static constexpr const wchar_t* value() noexcept { return L" \t\n\r\f\v"; }
};

template <typename TChar>
inline constexpr const TChar* whitespace_v = whitespace<TChar>::value();

} // namespace types
} // namespace castle