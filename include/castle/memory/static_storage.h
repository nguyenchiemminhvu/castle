#ifndef CASTLE_MEMORY_STATIC_STORAGE_H
#define CASTLE_MEMORY_STATIC_STORAGE_H

#include "castle/core/compiler.h"
#include "castle/memory/storage.h"

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

#endif // CASTLE_MEMORY_STATIC_STORAGE_H
