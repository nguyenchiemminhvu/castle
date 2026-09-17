#ifndef CASTLE_UTILITY_MOVE_HPP
#define CASTLE_UTILITY_MOVE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{

template <typename T>
CASTLE_CONSTEXPR meta::remove_reference_t<T>&& move(T&& value) CASTLE_NOEXCEPT
{
    return static_cast<meta::remove_reference_t<T>&&>(value);
}

} // namespace castle

#endif // CASTLE_UTILITY_MOVE_HPP
