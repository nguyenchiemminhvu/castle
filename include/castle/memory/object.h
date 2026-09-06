#ifndef CASTLE_MEMORY_OBJECT_H
#define CASTLE_MEMORY_OBJECT_H

#include "castle/core/compiler.h"
#include "castle/memory/construct.h"
#include "castle/memory/destroy.h"
#include "castle/memory/lifetime.h"

namespace castle
{
namespace memory
{

// Converts storage address to a typed pointer only after the object lifetime
// has been started. launder is used for C++17 lifetime correctness.
template <typename T>
T* object_from_address(void* address) CASTLE_NOEXCEPT
{
    return launder(reinterpret_cast<T*>(address));
}

template <typename T>
CASTLE_CONST T* object_from_address(CASTLE_CONST void* address) CASTLE_NOEXCEPT
{
    return launder(reinterpret_cast<CASTLE_CONST T*>(address));
}

} // namespace memory
} // namespace castle

#endif // CASTLE_MEMORY_OBJECT_H
