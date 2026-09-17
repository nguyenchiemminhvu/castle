// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file forward.hpp
 * @brief Perfect-forwarding helpers for Castle templates.
 *
 * Use this header when a generic function needs to preserve whether an input
 * argument arrived as an lvalue or rvalue without depending on the standard
 * library. The implementation is constexpr, heap-free, exception-free, and
 * only performs reference-category preservation.
 *
 * @code
 * template <typename T>
 * void relay(T&& value)
 * {
 *     sink(castle::forward<T>(value));
 * }
 * @endcode
 */
#ifndef CASTLE_UTILITY_FORWARD_HPP
#define CASTLE_UTILITY_FORWARD_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{

/**
 * @brief Preserves the value category of an lvalue argument.
 * @tparam T Target forwarding type.
 * @param value Argument to forward.
 * @return `value` cast to `T&&`.
 */
template <typename T>
CASTLE_CONSTEXPR T&& forward(meta::remove_reference_t<T>& value) CASTLE_NOEXCEPT
{
    return static_cast<T&&>(value);
}

/**
 * @brief Preserves the value category of an rvalue argument.
 * @tparam T Target forwarding type.
 * @param value Argument to forward.
 * @return `value` cast to `T&&`.
 * @warning `T` must not be an lvalue reference type.
 */
template <typename T>
CASTLE_CONSTEXPR T&& forward(meta::remove_reference_t<T>&& value) CASTLE_NOEXCEPT
{
    static_assert(!meta::is_lvalue_reference<T>::value,
                  "CASTLE_FORWARD: cannot forward an rvalue as an lvalue");
    return static_cast<T&&>(value);
}

} // namespace castle

#endif // CASTLE_UTILITY_FORWARD_HPP
