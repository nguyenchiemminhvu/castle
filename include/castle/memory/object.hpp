// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file object.hpp
 * @brief Converts raw addresses back into typed object pointers once lifetime is active.
 *
 * Use this header after an object has already been constructed in caller-managed storage and
 * code needs a typed pointer with C++17 lifetime laundering applied. It performs no allocation
 * and does not create or destroy objects on its own.
 *
 * @code
 * auto* pointer = castle::memory::object_from_address<MyType>(storage_address);
 * @endcode
 */
#ifndef CASTLE_MEMORY_OBJECT_HPP
#define CASTLE_MEMORY_OBJECT_HPP

#include "castle/core/compiler.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/lifetime.hpp"

namespace castle
{
namespace memory
{

/**
 * @brief Returns a mutable `T*` view of an address that already contains a live `T`.
 * @tparam T Object type stored at @p address.
 * @param address Address of storage whose active object is `T`.
 * @return Laundered pointer to the object at @p address.
 * @note This helper is typically used after placement construction or explicit storage reuse.
 * @warning `T` must already be alive at @p address, and the address must satisfy `alignof(T)`.
 */
template <typename T>
T* object_from_address(void* address) CASTLE_NOEXCEPT
{
    return launder(reinterpret_cast<T*>(address));
}

/**
 * @brief Returns a const `T*` view of an address that already contains a live `T`.
 * @tparam T Object type stored at @p address.
 * @param address Address of storage whose active object is `T`.
 * @return Laundered const pointer to the object at @p address.
 * @note This helper preserves const qualification while applying lifetime laundering.
 * @warning `T` must already be alive at @p address, and the address must satisfy `alignof(T)`.
 */
template <typename T>
CASTLE_CONST T* object_from_address(CASTLE_CONST void* address) CASTLE_NOEXCEPT
{
    return launder(reinterpret_cast<CASTLE_CONST T*>(address));
}

}
}

#endif // CASTLE_MEMORY_OBJECT_HPP
