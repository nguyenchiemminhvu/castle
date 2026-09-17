// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file static_storage.hpp
 * @brief Names a fixed-capacity block of raw storage for `T` objects.
 *
 * Use this header when code needs deterministic, preallocated slots for a known number of
 * objects without constructing them up front. The storage is heap-free, sized at compile time,
 * and leaves lifetime management entirely to the caller or container.
 *
 * @code
 * castle::memory::static_storage<uint32_t, 4U> storage;
 * void* first_slot = storage.address(0U);
 * @endcode
 */
#ifndef CASTLE_MEMORY_STATIC_STORAGE_HPP
#define CASTLE_MEMORY_STATIC_STORAGE_HPP

#include "castle/core/compiler.hpp"
#include "castle/memory/storage.hpp"

namespace castle
{
namespace memory
{

/**
 * @brief Alias for fixed-capacity raw storage of `N` `T`-sized slots.
 * @tparam T Type whose size and alignment define each slot.
 * @tparam N Number of slots in the storage.
 * @note This alias does not construct or destroy `T` objects automatically.
 * @warning Any object placed into the storage must be constructed, accessed, and destroyed
 * explicitly by the caller.
 */
template <typename T, size_t N>
using static_storage = raw_storage<T, N>;

}
}

#endif // CASTLE_MEMORY_STATIC_STORAGE_HPP
