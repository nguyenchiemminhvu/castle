#ifndef CASTLE_MEMORY_DESTROY_H
#define CASTLE_MEMORY_DESTROY_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

#include <stddef.h>

namespace castle
{
namespace memory
{

// Calling a trivial destructor is unnecessary. This also lets the same
// primitive work cleanly for scalar/trivial types such as uint32_t.
template <typename T>
void destroy_at(T* pointer) CASTLE_NOEXCEPT
{
    static_assert(meta::is_destructible<T>::value,
                  "T must be destructible");

    if CASTLE_CONSTEXPR (!meta::is_trivially_destructible<T>::value)
    {
        pointer->~T();
    }
}

template <typename T>
void destroy_n(T* pointer, size_t count) CASTLE_NOEXCEPT
{
    for (size_t i = count; i > 0U; --i)
    {
        destroy_at(pointer + (i - 1U));
    }
}

} // namespace memory
} // namespace castle

#endif // CASTLE_MEMORY_DESTROY_H
