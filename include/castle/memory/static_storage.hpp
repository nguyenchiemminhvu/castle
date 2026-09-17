#ifndef CASTLE_MEMORY_STATIC_STORAGE_HPP
#define CASTLE_MEMORY_STATIC_STORAGE_HPP

#include "castle/core/compiler.hpp"
#include "castle/memory/storage.hpp"

namespace castle
{
namespace memory
{

// Named public type for fixed-capacity object storage. Lifetime remains the
// caller/container's responsibility; no constructor for T is invoked here.
template <typename T, size_t N>
using static_storage = raw_storage<T, N>;

} // namespace memory
} // namespace castle

#endif // CASTLE_MEMORY_STATIC_STORAGE_HPP
