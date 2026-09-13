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

#endif // CASTLE_MEMORY_CONSTRUCT_HPP
