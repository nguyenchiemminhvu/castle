#ifndef CASTLE_MEMORY_ADDRESOF_HPP
#define CASTLE_MEMORY_ADDRESOF_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace memory
{

template <typename T>
CASTLE_CONSTEXPR T* addressof(T& value) CASTLE_NOEXCEPT
{
    return reinterpret_cast<T*>(
        &const_cast<char&>(
            reinterpret_cast<CASTLE_CONST CASTLE_VOLATILE char&>(value)
        )
    );
}

template <typename T>
CASTLE_CONSTEXPR T CASTLE_CONST* addressof(T CASTLE_CONST& value) CASTLE_NOEXCEPT
{
    return reinterpret_cast<T CASTLE_CONST*>(
        &const_cast<char&>(
            reinterpret_cast<CASTLE_CONST CASTLE_VOLATILE char&>(value)
        )
    );
}

} // namespace memory
} // namespace castle

#endif // CASTLE_MEMORY_ADDRESOF_HPP