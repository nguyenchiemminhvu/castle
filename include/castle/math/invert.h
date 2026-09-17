#ifndef CASTLE_MATH_INVERT_H
#define CASTLE_MATH_INVERT_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/core/type_ranges.h"

namespace castle
{
namespace math
{

template <typename T>
class invert
{
    static_assert(meta::is_arithmetic<T>::value,
                  "castle::math::invert requires an arithmetic type");

public:
    CASTLE_CONSTEXPR invert() CASTLE_NOEXCEPT
        : offset_(T{0})
        , minuend_(meta::is_signed<T>::value
                   ? T{0}
                   : castle::numeric_limits<T>::max())
    {
    }

    CASTLE_CONSTEXPR invert(T offset, T minuend) CASTLE_NOEXCEPT
        : offset_(offset)
        , minuend_(minuend)
    {
    }

    CASTLE_CONSTEXPR T operator()(T value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        // (offset_ + minuend_) - value = minuend_ - (value - offset_)
        return static_cast<T>(minuend_ - static_cast<T>(value - offset_));
    }

    CASTLE_CONSTEXPR T offset() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return offset_;
    }

    CASTLE_CONSTEXPR T minuend() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return minuend_;
    }

private:
    T offset_;
    T minuend_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_INVERT_H
