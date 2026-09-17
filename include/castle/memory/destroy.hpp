// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file destroy.hpp
 * @brief Ends object lifetime explicitly in caller-managed storage.
 *
 * Use this header alongside manual construction helpers when objects live inside static,
 * stack, or other preallocated buffers. No deallocation occurs; the functions only invoke
 * destructors when needed. Call them only for objects whose lifetime is currently active.
 *
 * @code
 * castle::memory::destroy_at(pointer);
 * castle::memory::destroy_n(first, count);
 * @endcode
 */
#ifndef CASTLE_MEMORY_DESTROY_HPP
#define CASTLE_MEMORY_DESTROY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

#include <stddef.h>

namespace castle
{
namespace memory
{

/**
 * @brief Destroys the object pointed to by @p pointer when destruction is required.
 * @tparam T Object type.
 * @param pointer Pointer to a live `T` object.
 * @return Nothing.
 * @note Trivially destructible types are ignored, so this function is a no-op for them.
 * @warning For non-trivially destructible `T`, @p pointer must refer to a live object of type
 * `T` whose lifetime should end exactly once.
 */
template <typename T>
void destroy_at(CASTLE_UNUSED T* pointer) CASTLE_NOEXCEPT
{
    static_assert(meta::is_destructible<T>::value,
                  "T must be destructible");

    if CASTLE_CONSTEXPR (!meta::is_trivially_destructible<T>::value)
    {
        pointer->~T();
    }
}

/**
 * @brief Destroys `count` consecutive live objects starting at @p pointer.
 * @tparam T Object type.
 * @param pointer Pointer to the first live object in the range.
 * @param count Number of objects to destroy.
 * @return Nothing.
 * @note Objects are destroyed in reverse order, matching normal array teardown order.
 * @warning The range `[pointer, pointer + count)` must contain live `T` objects whose
 * lifetimes should end exactly once.
 */
template <typename T>
void destroy_n(T* pointer, size_t count) CASTLE_NOEXCEPT
{
    for (size_t i = count; i > 0U; --i) // Reverse order preserves normal array-like destruction sequencing.
    {
        destroy_at(pointer + (i - 1U));
    }
}

}
}

#endif // CASTLE_MEMORY_DESTROY_HPP
