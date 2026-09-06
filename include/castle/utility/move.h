#ifndef CASTLE_UTILITY_MOVE_H
#define CASTLE_UTILITY_MOVE_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{

template <typename T>
CASTLE_CONSTEXPR meta::remove_reference_t<T>&& move(T&& value) CASTLE_NOEXCEPT
{
    return static_cast<meta::remove_reference_t<T>&&>(value);
}

} // namespace castle

#endif // CASTLE_UTILITY_MOVE_H
