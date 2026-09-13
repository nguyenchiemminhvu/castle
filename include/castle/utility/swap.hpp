#ifndef CASTLE_UTILITY_SWAP_HPP
#define CASTLE_UTILITY_SWAP_HPP

#include "castle/core/compiler.hpp"
#include "castle/utility/move.hpp"

namespace castle
{

template <typename T>
CASTLE_INLINE void swap(T& lhs, T& rhs) CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(T(CASTLE_MOVE(lhs))) &&
                                                        CASTLE_NOEXCEPT(lhs = CASTLE_MOVE(rhs)) &&
                                                        CASTLE_NOEXCEPT(rhs = CASTLE_MOVE(lhs)) &&
                                                        meta::is_nothrow_destructible<T>::value)
{
    T tmp(CASTLE_MOVE(lhs));
    lhs = CASTLE_MOVE(rhs);
    rhs = CASTLE_MOVE(tmp);
}

} // namespace castle

#endif // CASTLE_UTILITY_SWAP_HPP
