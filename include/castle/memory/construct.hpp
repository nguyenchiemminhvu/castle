// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file construct.hpp
 * @brief Starts object lifetime in caller-supplied storage with placement construction.
 *
 * Use this header when raw bytes have already been reserved and an object must be constructed
 * in place without dynamic allocation. The target address must provide enough size and
 * alignment for `T`, and callers remain responsible for eventually destroying the object.
 *
 * @code
 * castle::aligned_storage_as_t<sizeof(uint32_t), uint32_t> storage;
 * uint32_t* value = castle::memory::construct_at<uint32_t>(storage.get_address<uint32_t>(), 7U);
 * @endcode
 */
#ifndef CASTLE_MEMORY_CONSTRUCT_HPP
#define CASTLE_MEMORY_CONSTRUCT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/utility/forward.hpp"
#include "castle/memory/new.hpp"

namespace castle
{
namespace memory
{

/**
 * @brief Constructs `T` at a caller-provided address.
 * @tparam T Object type to construct.
 * @tparam Args Constructor argument types.
 * @param address Storage where the object will be created.
 * @param args Arguments forwarded to `T`'s constructor.
 * @return Pointer to the newly constructed object.
 * @note This function performs placement construction only; it never allocates memory.
 * @warning @p address must be non-null, sufficiently aligned for `T`, and refer to storage
 * large enough to hold `T`. Constructing over an unrelated live object without first ending
 * that object's lifetime is undefined behavior.
 */
template <typename T, typename... Args>
T* construct_at(void* address, Args&&... args)
    CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
{
    static_assert(meta::is_constructible<T, Args&&...>::value,
                  "T cannot be constructed from the supplied arguments");
    return ::new (address) T(CASTLE_FORWARD<Args>(args)...); // Placement new starts T's lifetime in caller-owned storage.
}

}
}

#endif // CASTLE_MEMORY_CONSTRUCT_HPP
