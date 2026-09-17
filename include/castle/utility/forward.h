#ifndef CASTLE_UTILITY_FORWARD_H
#define CASTLE_UTILITY_FORWARD_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{

template <typename T>
CASTLE_CONSTEXPR T&& forward(meta::remove_reference_t<T>& value) CASTLE_NOEXCEPT
{
    return static_cast<T&&>(value);
}

template <typename T>
CASTLE_CONSTEXPR T&& forward(meta::remove_reference_t<T>&& value) CASTLE_NOEXCEPT
{
    static_assert(!meta::is_lvalue_reference<T>::value,
                  "CASTLE_FORWARD: cannot forward an rvalue as an lvalue");
    return static_cast<T&&>(value);
}

} // namespace castle

#endif // CASTLE_UTILITY_FORWARD_H
