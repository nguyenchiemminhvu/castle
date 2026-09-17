#ifndef CASTLE_MATH_LOGARITHM_H
#define CASTLE_MATH_LOGARITHM_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"

namespace castle
{
namespace math
{

// ===========================================================================
// logarithm
//
// Compile-time integer logarithm.
// Result is rounded down to the nearest integer.
//
// Example:
//   logarithm<8, 2>::value    == 3
//   logarithm<15, 2>::value   == 3
//   logarithm<1000, 10>::value == 3
// ===========================================================================
template <size_type Value, size_type Base>
struct logarithm
{
    static_assert(Base > 1, "Base must be greater than 1");
    static_assert(Value > 0, "Value must be greater than 0");

    static constexpr size_type value =
        (Value >= Base)
        ? (1U + logarithm<Value / Base, Base>::value)
        : 0U;
};

// ===========================================================================
// Specialization for Value = 1
// ===========================================================================
template <size_type Base>
struct logarithm<1U, Base>
{
    static_assert(Base > 1, "Base must be greater than 1");

    static constexpr size_type value = 0U;
};

// ===========================================================================
// Specialization for Value = 0
// ===========================================================================
template <size_type Base>
struct logarithm<0U, Base>
{
    static_assert(Base > 1, "Base must be greater than 1");

    static constexpr size_type value = 0U;
};

// ===========================================================================
// log2
// ===========================================================================
template <size_type Value>
struct log2
{
    static_assert(Value > 0, "Value must be greater than 0");

    static constexpr size_type value = logarithm<Value, 2U>::value;
};

// ===========================================================================
// log10
// ===========================================================================
template <size_type Value>
struct log10
{
    static_assert(Value > 0, "Value must be greater than 0");

    static constexpr size_type value = logarithm<Value, 10U>::value;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_LOGARITHM_H