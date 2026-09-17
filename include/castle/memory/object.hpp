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

#endif // CASTLE_MEMORY_OBJECT_HPP
