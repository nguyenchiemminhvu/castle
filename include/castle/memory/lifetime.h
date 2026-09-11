#ifndef CASTLE_MEMORY_LIFETIME_H
#define CASTLE_MEMORY_LIFETIME_H

#include "castle/core/compiler.h"

namespace castle
{
namespace memory
{

// C++17 object-lifetime helper. GCC and Clang expose __builtin_launder; using
// it keeps CASTLE independent of the standard lifetime-laundering facility while preserving the C++17
// lifetime model for objects created inside raw storage.
template <typename T>
T* launder(T* pointer) CASTLE_NOEXCEPT
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_launder(pointer);
#else
    return pointer;
#endif
}

template <typename T>
CASTLE_CONST T* launder(CASTLE_CONST T* pointer) CASTLE_NOEXCEPT
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_launder(pointer);
#else
    return pointer;
#endif
}

} // namespace memory
} // namespace castle

#endif // CASTLE_MEMORY_LIFETIME_H
