// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file addressof.hpp
 * @brief Obtains the true address of an object without invoking an overloaded operator&.
 *
 * Use this header in low-level storage, container, and lifetime code when a raw pointer to
 * an existing object is required. It performs no allocation, does not alter object lifetime,
 * and is deterministic for embedded targets.
 *
 * @code
 * uint32_t value = 7U;
 * uint32_t* pointer = castle::memory::addressof(value);
 * @endcode
 */
#ifndef CASTLE_MEMORY_ADDRESOF_HPP
#define CASTLE_MEMORY_ADDRESOF_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace memory
{

/**
 * @brief Returns the actual address of a mutable object.
 * @tparam T Object type.
 * @param value Object whose address is required.
 * @return Pointer to @p value.
 * @note This helper bypasses overloaded address-of operators.
 * @warning @p value must denote a valid object.
 */
template <typename T>
CASTLE_CONSTEXPR T* addressof(T& value) CASTLE_NOEXCEPT
{
    return reinterpret_cast<T*>(
        &const_cast<char&>(
            reinterpret_cast<CASTLE_CONST CASTLE_VOLATILE char&>(value)
        )
    );
}

/**
 * @brief Returns the actual address of a const object.
 * @tparam T Object type.
 * @param value Object whose address is required.
 * @return Pointer to @p value with const qualification preserved.
 * @note This helper bypasses overloaded address-of operators.
 * @warning @p value must denote a valid object.
 */
template <typename T>
CASTLE_CONSTEXPR T CASTLE_CONST* addressof(T CASTLE_CONST& value) CASTLE_NOEXCEPT
{
    return reinterpret_cast<T CASTLE_CONST*>(
        &const_cast<char&>(
            reinterpret_cast<CASTLE_CONST CASTLE_VOLATILE char&>(value)
        )
    );
}

}
}

#endif // CASTLE_MEMORY_ADDRESOF_HPP
