// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file lifetime.hpp
 * @brief Provides lifetime laundering helpers for objects created in raw storage.
 *
 * Use this header after placement construction or storage reuse when C++17 lifetime rules
 * require a laundered pointer to access the active object. No allocation occurs, and the
 * returned pointer always names the same storage address.
 *
 * @code
 * auto* pointer = castle::memory::launder(existing_pointer);
 * @endcode
 */
#ifndef CASTLE_MEMORY_LIFETIME_HPP
#define CASTLE_MEMORY_LIFETIME_HPP

#include "castle/core/compiler.hpp"

namespace castle
{
namespace memory
{

/**
 * @brief Returns a lifetime-correct mutable pointer to the object at @p pointer.
 * @tparam T Object type.
 * @param pointer Pointer to the active object.
 * @return Laundered pointer to the same address.
 * @note Uses `__builtin_launder` on compilers that provide it.
 * @warning @p pointer should refer to an object whose lifetime is active, especially after
 * in-place reconstruction in reused storage.
 */
template <typename T>
T* launder(T* pointer) CASTLE_NOEXCEPT
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_launder(pointer);
#else
    return pointer;
#endif
}

/**
 * @brief Returns a lifetime-correct const pointer to the object at @p pointer.
 * @tparam T Object type.
 * @param pointer Pointer to the active object.
 * @return Laundered const pointer to the same address.
 * @note Uses `__builtin_launder` on compilers that provide it.
 * @warning @p pointer should refer to an object whose lifetime is active, especially after
 * in-place reconstruction in reused storage.
 */
template <typename T>
CASTLE_CONST T* launder(CASTLE_CONST T* pointer) CASTLE_NOEXCEPT
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_launder(pointer);
#else
    return pointer;
#endif
}

}
}

#endif // CASTLE_MEMORY_LIFETIME_HPP
