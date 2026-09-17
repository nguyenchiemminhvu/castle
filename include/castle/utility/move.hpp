// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file move.hpp
 * @brief Move-cast helper for Castle value types.
 *
 * Use this header when ownership or resources should be transferred from a
 * named object without including `<utility>`. The helper performs a pure cast,
 * never allocates, never throws, and does not move any data by itself.
 *
 * @code
 * packet next = castle::move(current);
 * @endcode
 */
#ifndef CASTLE_UTILITY_MOVE_HPP
#define CASTLE_UTILITY_MOVE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{

/**
 * @brief Converts an expression to an rvalue reference of its base type.
 * @tparam T Expression type to cast.
 * @param value Expression to cast.
 * @return `value` cast to `remove_reference_t<T>&&`.
 */
template <typename T>
CASTLE_CONSTEXPR meta::remove_reference_t<T>&& move(T&& value) CASTLE_NOEXCEPT
{
    return static_cast<meta::remove_reference_t<T>&&>(value);
}

} // namespace castle

#endif // CASTLE_UTILITY_MOVE_HPP
