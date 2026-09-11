#ifndef CASTLE_MEMORY_CONSTRUCT_H
#define CASTLE_MEMORY_CONSTRUCT_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/utility/forward.h"
#include "castle/memory/new.h"

namespace castle
{
namespace memory
{

// Placement construction is the only operation that starts a T lifetime in
// raw storage. It performs no allocation.
template <typename T, typename... Args>
T* construct_at(void* address, Args&&... args)
    CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
{
    static_assert(meta::is_constructible<T, Args&&...>::value,
                  "T cannot be constructed from the supplied arguments");
    return ::new (address) T(CASTLE_FORWARD<Args>(args)...);
}

} // namespace memory
} // namespace castle

#endif // CASTLE_MEMORY_CONSTRUCT_H
