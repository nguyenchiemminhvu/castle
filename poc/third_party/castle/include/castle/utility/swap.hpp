// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file swap.hpp
 * @brief Heap-free swap helper built on Castle move semantics.
 *
 * Use this header to exchange two objects when the project avoids the standard
 * library. The implementation constructs one temporary object on the stack,
 * performs no allocation, and is noexcept when the moved type supports it.
 *
 * @code
 * castle::swap(lhs, rhs);
 * @endcode
 */
#ifndef CASTLE_UTILITY_SWAP_HPP
#define CASTLE_UTILITY_SWAP_HPP

#include "castle/core/compiler.hpp"
#include "castle/utility/move.hpp"

namespace castle
{

/**
 * @brief Exchanges the values of two objects.
 * @tparam T Object type to swap.
 * @param lhs First object.
 * @param rhs Second object.
 * @note The implementation uses move construction and move assignment.
 */
template <typename T>
CASTLE_INLINE void swap(T& lhs, T& rhs) CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(T(CASTLE_MOVE(lhs))) &&
                                                        CASTLE_NOEXCEPT(lhs = CASTLE_MOVE(rhs)) &&
                                                        CASTLE_NOEXCEPT(rhs = CASTLE_MOVE(lhs)) &&
                                                        meta::is_nothrow_destructible<T>::value)
{
    T tmp(CASTLE_MOVE(lhs));
    lhs = CASTLE_MOVE(rhs);
    rhs = CASTLE_MOVE(tmp);
}

} // namespace castle

#endif // CASTLE_UTILITY_SWAP_HPP
