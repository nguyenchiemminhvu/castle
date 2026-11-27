// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file quantity.hpp
 * @brief Strongly typed physical quantities with compile-time dimensional analysis.
 *
 * A `quantity<Unit, Rep>` stores one arithmetic value of type `Rep` together with
 * a compile-time `Unit`. The unit records:
 * - the physical dimension as integer exponents of the SI base dimensions plus
 *   plane angle (so `rad/s` and `Hz` stay distinct),
 * - an exact `castle::math::ratio` scale relative to the coherent SI unit,
 * - an optional power of pi (so degrees and radians convert without rounding
 *   the scale itself),
 * - an optional origin offset for affine units such as degrees Celsius,
 * - an optional kind tag that separates quantities of the same dimension that
 *   must not mix (for example geographic coordinates and plane angles).
 *
 * Quantities of incompatible dimensions or kinds cannot be added, compared or
 * assigned. Multiplication and division derive the resulting unit, so
 * `meters / seconds` yields `meters_per_second`. Results whose dimension is
 * pure time are `castle::chrono::duration` values in seconds, and dimensionless
 * results collapse to a plain arithmetic value; both are expressed in the
 * coherent unit (scale 1), so they can be used without further conversion.
 *
 * Example:
 * @code
 * #include "castle/units/distance.hpp"
 * #include "castle/units/velocity.hpp"
 * #include "castle/units/time.hpp"
 *
 * void set_velocity(castle::units::meters_per_second<double> speed);
 *
 * castle::units::kilometers<double> leg(1.5);
 * castle::units::seconds<double> elapsed(60.0);
 * set_velocity(leg / elapsed); // 25 m/s
 * @endcode
 *
 * Key constraints:
 * - No heap, exceptions, RTTI or virtual dispatch; a quantity is exactly as
 *   large as its `Rep`.
 * - Conversions are implicit only when lossless: a floating-point destination,
 *   or an integral destination whose scale factor and offset are whole numbers.
 *   Every other conversion needs an explicit constructor or `quantity_cast`.
 * - Integral conversions truncate toward zero. Arithmetic is unchecked and may
 *   overflow the representation, like `castle::chrono::duration`.
 */
#ifndef CASTLE_UNITS_QUANTITY_HPP
#define CASTLE_UNITS_QUANTITY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/chrono/duration.hpp"
#include "castle/math/gcd.hpp"
#include "castle/math/lcm.hpp"
#include "castle/math/ratio.hpp"
#include "castle/units/scale.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{

/**
 * @brief Compile-time physical dimension expressed as base-dimension exponents.
 * @tparam Length Exponent of length (L).
 * @tparam Mass Exponent of mass (M).
 * @tparam Time Exponent of time (T).
 * @tparam Current Exponent of electric current (I).
 * @tparam Temperature Exponent of thermodynamic temperature (Theta).
 * @tparam Amount Exponent of amount of substance (N).
 * @tparam Luminous Exponent of luminous intensity (J).
 * @tparam Angle Exponent of plane angle; kept separate so rotational and
 *         cyclic quantities do not mix.
 */
template <int Length, int Mass, int Time, int Current,
          int Temperature, int Amount, int Luminous, int Angle>
struct dimension
{
    static CASTLE_CONSTEXPR int length      = Length;      ///< Length exponent.
    static CASTLE_CONSTEXPR int mass        = Mass;        ///< Mass exponent.
    static CASTLE_CONSTEXPR int time        = Time;        ///< Time exponent.
    static CASTLE_CONSTEXPR int current     = Current;     ///< Electric current exponent.
    static CASTLE_CONSTEXPR int temperature = Temperature; ///< Temperature exponent.
    static CASTLE_CONSTEXPR int amount      = Amount;      ///< Amount-of-substance exponent.
    static CASTLE_CONSTEXPR int luminous    = Luminous;    ///< Luminous-intensity exponent.
    static CASTLE_CONSTEXPR int angle       = Angle;       ///< Plane-angle exponent.
};

/**
 * @brief Product of two dimensions; exponents are added.
 * @tparam D1 Left-hand dimension.
 * @tparam D2 Right-hand dimension.
 */
template <typename D1, typename D2>
struct dimension_multiply;

template <int L1, int M1, int T1, int I1, int Th1, int N1, int J1, int A1,
          int L2, int M2, int T2, int I2, int Th2, int N2, int J2, int A2>
struct dimension_multiply<dimension<L1, M1, T1, I1, Th1, N1, J1, A1>,
                          dimension<L2, M2, T2, I2, Th2, N2, J2, A2>>
{
    using type = dimension<L1 + L2, M1 + M2, T1 + T2, I1 + I2,
                           Th1 + Th2, N1 + N2, J1 + J2, A1 + A2>;
};

/**
 * @brief Alias for the result of `dimension_multiply`.
 */
template <typename D1, typename D2>
using dimension_multiply_t = typename dimension_multiply<D1, D2>::type;

/**
 * @brief Reciprocal of a dimension; exponents are negated.
 * @tparam D Dimension to invert.
 */
template <typename D>
struct dimension_inverse;

template <int L, int M, int T, int I, int Th, int N, int J, int A>
struct dimension_inverse<dimension<L, M, T, I, Th, N, J, A>>
{
    using type = dimension<-L, -M, -T, -I, -Th, -N, -J, -A>;
};

/**
 * @brief Alias for the result of `dimension_inverse`.
 */
template <typename D>
using dimension_inverse_t = typename dimension_inverse<D>::type;

/**
 * @brief Quotient of two dimensions; exponents are subtracted.
 */
template <typename D1, typename D2>
using dimension_divide_t = dimension_multiply_t<D1, dimension_inverse_t<D2>>;

/**
 * @brief Base dimensions. Derived dimensions are declared by the quantity headers.
 */
namespace dim
{
using dimensionless      = dimension<0, 0, 0, 0, 0, 0, 0, 0>; ///< Pure number.
using length             = dimension<1, 0, 0, 0, 0, 0, 0, 0>; ///< L.
using mass               = dimension<0, 1, 0, 0, 0, 0, 0, 0>; ///< M.
using time               = dimension<0, 0, 1, 0, 0, 0, 0, 0>; ///< T.
using current            = dimension<0, 0, 0, 1, 0, 0, 0, 0>; ///< I.
using temperature        = dimension<0, 0, 0, 0, 1, 0, 0, 0>; ///< Theta.
using amount             = dimension<0, 0, 0, 0, 0, 1, 0, 0>; ///< N.
using luminous_intensity = dimension<0, 0, 0, 0, 0, 0, 1, 0>; ///< J.
using angle              = dimension<0, 0, 0, 0, 0, 0, 0, 1>; ///< Plane angle.
} // namespace dim

/**
 * @brief Canonical unit descriptor. Use the `unit` alias to build one.
 * @tparam Dimension Physical dimension.
 * @tparam Ratio Reduced, positive scale relative to the coherent SI unit.
 * @tparam PiExponent Power of pi multiplied into the scale.
 * @tparam Offset Reduced origin offset in coherent SI units:
 *         `coherent = value * scale + offset`.
 * @tparam Kind Tag separating quantities of the same dimension, or `void`.
 */
template <typename Dimension, typename Ratio, int PiExponent, typename Offset, typename Kind>
struct basic_unit
{
    static_assert((Ratio::num > 0) && (Ratio::den > 0),
                  "castle::units::unit scale must be positive");
    static_assert((Offset::num == 0) || (PiExponent == 0),
                  "castle::units::unit with an offset cannot carry a pi factor");

    using dimension = Dimension;                                ///< Physical dimension.
    using ratio = Ratio;                                        ///< Scale relative to the coherent unit.
    static CASTLE_CONSTEXPR int pi_exponent = PiExponent;       ///< Power of pi in the scale.
    using offset = Offset;                                      ///< Origin offset in coherent units.
    using kind = Kind;                                          ///< Kind tag or `void`.
};

/**
 * @brief Builds a canonical unit; ratios are reduced so equal units are the same type.
 * @tparam Dimension Physical dimension.
 * @tparam Ratio Scale relative to the coherent SI unit. Defaults to `1`.
 * @tparam PiExponent Power of pi multiplied into the scale. Defaults to `0`.
 * @tparam Offset Origin offset in coherent SI units. Defaults to `0`.
 * @tparam Kind Kind tag. Defaults to `void`.
 */
template <typename Dimension,
          typename Ratio = math::ratio<1>,
          int PiExponent = 0,
          typename Offset = math::ratio<0>,
          typename Kind = void>
using unit = basic_unit<Dimension, typename Ratio::type, PiExponent, typename Offset::type, Kind>;

/**
 * @brief Unit scaled by an extra ratio, keeping dimension, pi factor, offset and kind.
 * @tparam Unit Unit to scale.
 * @tparam Ratio Extra scale, for example `units::milli` or `math::ratio<1, 10000000>`.
 */
template <typename Unit, typename Ratio>
using scaled_unit_t = unit<typename Unit::dimension,
                           math::ratio_multiply_t<typename Unit::ratio, Ratio>,
                           Unit::pi_exponent,
                           typename Unit::offset,
                           typename Unit::kind>;

/**
 * @brief Coherent SI unit (scale 1, no pi factor, no offset) with the same dimension and kind.
 * @tparam Unit Source unit.
 */
template <typename Unit>
using coherent_unit_t = unit<typename Unit::dimension,
                             math::ratio<1>,
                             0,
                             math::ratio<0>,
                             typename Unit::kind>;

template <typename Unit, typename Rep>
class quantity;

/**
 * @brief Reports whether a type is a `castle::units::quantity`.
 * @tparam T Type to test.
 */
template <typename T>
struct is_quantity : meta::false_type
{
};

template <typename Unit, typename Rep>
struct is_quantity<quantity<Unit, Rep>> : meta::true_type
{
};

namespace detail
{

template <typename T>
struct is_unit : meta::false_type
{
};

template <typename Dimension, typename Ratio, int PiExponent, typename Offset, typename Kind>
struct is_unit<basic_unit<Dimension, Ratio, PiExponent, Offset, Kind>> : meta::true_type
{
};

/**
 * @brief Floating type used when a conversion involves pi or an offset fallback.
 * @note The common floating type of both operands, or `double` when both are integral.
 */
template <typename Rep1, typename Rep2>
using floating_rep_t = meta::conditional_t<
    meta::is_floating_point<meta::common_type_t<Rep1, Rep2>>::value,
    meta::common_type_t<Rep1, Rep2>,
    double>;

/**
 * @brief Returns pi raised to an integer power.
 * @tparam T Floating-point result type.
 * @param exponent Power of pi, may be negative.
 * @return `pi^exponent`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T pi_power(int exponent) CASTLE_NOEXCEPT
{
    T result = static_cast<T>(1);
    for (int i = 0; i < exponent; ++i)
    {
        result *= math::pi<T>();
    }
    for (int i = 0; i > exponent; --i)
    {
        result /= math::pi<T>();
    }
    return result;
}

/**
 * @brief `R1 / R2`, cancelling common factors before multiplying.
 * @note Large but mutually reducible scales (e.g. pound-force against the
 *       common gcd/lcm scale) then stay within `intmax_t`.
 */
template <typename R1, typename R2>
struct ratio_divide_reduced
{
private:
    static CASTLE_CONSTEXPR intmax_t num_gcd = math::gcd<R1::num, R2::num>::value;
    static CASTLE_CONSTEXPR intmax_t den_gcd = math::gcd<R1::den, R2::den>::value;

public:
    using type = math::ratio<(R1::num / num_gcd) * (R2::den / den_gcd),
                             (R1::den / den_gcd) * (R2::num / num_gcd)>;
};

/**
 * @brief Compile-time description of the affine map from one unit to another.
 * @note `to = from * factor * pi^pi_exponent + shift`.
 */
template <typename FromUnit, typename ToUnit>
struct conversion
{
    using factor = typename ratio_divide_reduced<typename FromUnit::ratio, typename ToUnit::ratio>::type;
    using shift = typename ratio_divide_reduced<
        math::ratio_subtract_t<typename FromUnit::offset, typename ToUnit::offset>,
        typename ToUnit::ratio>::type;

    static CASTLE_CONSTEXPR int pi_exponent = FromUnit::pi_exponent - ToUnit::pi_exponent;

    /// @brief True when an integral value maps to an integral value exactly.
    static CASTLE_CONSTEXPR bool is_exact_integral =
        (factor::den == 1) && (shift::den == 1) && (pi_exponent == 0);
};

template <typename Unit1, typename Unit2>
struct same_dimension_and_kind
    : meta::bool_constant<meta::is_same<typename Unit1::dimension, typename Unit2::dimension>::value &&
                          meta::is_same<typename Unit1::kind, typename Unit2::kind>::value>
{
};

/**
 * @brief Converts a raw value between two units of the same dimension.
 * @tparam FromUnit Source unit.
 * @tparam ToUnit Destination unit.
 * @tparam ToRep Destination representation.
 * @tparam FromRep Source representation.
 * @param value Value expressed in `FromUnit`.
 * @return Value expressed in `ToUnit`; integral results truncate toward zero.
 */
template <typename FromUnit, typename ToUnit, typename ToRep, typename FromRep>
CASTLE_NODISCARD CASTLE_CONSTEXPR ToRep convert_value(FromRep value) CASTLE_NOEXCEPT
{
    static_assert(meta::is_same<typename FromUnit::dimension, typename ToUnit::dimension>::value,
                  "castle::units: conversion between different dimensions");

    using conv = conversion<FromUnit, ToUnit>;
    using factor = typename conv::factor;
    using shift = typename conv::shift;

    CASTLE_IF_CONSTEXPR (conv::pi_exponent != 0)
    {
        using calc = floating_rep_t<FromRep, ToRep>;
        return static_cast<ToRep>(
            static_cast<calc>(value) * static_cast<calc>(factor::num) / static_cast<calc>(factor::den) *
            pi_power<calc>(conv::pi_exponent));
    }
    else CASTLE_IF_CONSTEXPR (shift::num == 0)
    {
        using calc = meta::common_type_t<FromRep, ToRep, intmax_t>;
        return static_cast<ToRep>(
            (static_cast<calc>(value) * static_cast<calc>(factor::num)) / static_cast<calc>(factor::den));
    }
    else
    {
        // Single division at the end keeps integral conversions to one truncation.
        using calc = meta::common_type_t<FromRep, ToRep, intmax_t>;
        return static_cast<ToRep>(
            ((static_cast<calc>(value) * static_cast<calc>(factor::num) * static_cast<calc>(shift::den)) +
             (static_cast<calc>(shift::num) * static_cast<calc>(factor::den))) /
            (static_cast<calc>(factor::den) * static_cast<calc>(shift::den)));
    }
}

template <typename FromUnit, typename FromRep, typename ToUnit, typename ToRep>
struct is_lossless_impl
    : meta::bool_constant<meta::is_floating_point<ToRep>::value ||
                          (!meta::is_floating_point<FromRep>::value &&
                           conversion<FromUnit, ToUnit>::is_exact_integral)>
{
};

/**
 * @brief True when a value converts implicitly: same dimension and kind, and no precision loss.
 */
template <typename FromUnit, typename FromRep, typename ToUnit, typename ToRep>
struct is_lossless_conversion
    : meta::conditional_t<same_dimension_and_kind<FromUnit, ToUnit>::value,
                          is_lossless_impl<FromUnit, FromRep, ToUnit, ToRep>,
                          meta::false_type>
{
};

/**
 * @brief Selects the type produced by arithmetic for a unit.
 * @note Pure time maps to `chrono::duration` in seconds, dimensionless (kind `void`)
 *       maps to the plain representation; both are converted to coherent scale.
 *       Everything else stays a `quantity` with the derived scale.
 */
template <typename Unit, typename Rep,
          int Category =
              (meta::is_same<typename Unit::dimension, dim::time>::value &&
               meta::is_same<typename Unit::kind, void>::value) ? 1 :
              (meta::is_same<typename Unit::dimension, dim::dimensionless>::value &&
               meta::is_same<typename Unit::kind, void>::value) ? 2 : 0>
struct quantity_factory
{
    using type = quantity<Unit, Rep>;

    static CASTLE_CONSTEXPR type make(Rep value) CASTLE_NOEXCEPT
    {
        return type(value);
    }
};

template <typename Unit, typename Rep>
struct quantity_factory<Unit, Rep, 1>
{
    using type = chrono::duration<Rep, second>;

    static CASTLE_CONSTEXPR type make(Rep value) CASTLE_NOEXCEPT
    {
        return type(convert_value<Unit, coherent_unit_t<Unit>, Rep>(value));
    }
};

template <typename Unit, typename Rep>
struct quantity_factory<Unit, Rep, 2>
{
    using type = Rep;

    static CASTLE_CONSTEXPR type make(Rep value) CASTLE_NOEXCEPT
    {
        return convert_value<Unit, coherent_unit_t<Unit>, Rep>(value);
    }
};

/**
 * @brief Uniform access to quantities and chrono durations.
 */
template <typename T>
struct quantity_like : meta::false_type
{
};

template <typename Unit, typename Rep>
struct quantity_like<quantity<Unit, Rep>> : meta::true_type
{
    using unit_type = Unit;
    using rep = Rep;

    static CASTLE_CONSTEXPR Rep value_of(CASTLE_CONST quantity<Unit, Rep>& q) CASTLE_NOEXCEPT
    {
        return q.value();
    }
};

template <typename Rep, typename Period>
struct quantity_like<chrono::duration<Rep, Period>> : meta::true_type
{
    using unit_type = unit<dim::time, Period>;
    using rep = Rep;

    static CASTLE_CONSTEXPR Rep value_of(CASTLE_CONST chrono::duration<Rep, Period>& d) CASTLE_NOEXCEPT
    {
        return d.count();
    }
};

/**
 * @brief Enables quantity-by-quantity products: both operands quantity-like, at least one a quantity.
 */
template <typename A, typename B>
struct is_product_operands
    : meta::bool_constant<quantity_like<A>::value && quantity_like<B>::value &&
                          (is_quantity<A>::value || is_quantity<B>::value)>
{
};

template <typename A, typename B>
struct product
{
    using unit_a = typename quantity_like<A>::unit_type;
    using unit_b = typename quantity_like<B>::unit_type;
    using unit_type = unit<dimension_multiply_t<typename unit_a::dimension, typename unit_b::dimension>,
                           math::ratio_multiply_t<typename unit_a::ratio, typename unit_b::ratio>,
                           unit_a::pi_exponent + unit_b::pi_exponent>;
    using rep = meta::common_type_t<typename quantity_like<A>::rep, typename quantity_like<B>::rep>;
    using factory = quantity_factory<unit_type, rep>;
    using type = typename factory::type;
    static CASTLE_CONSTEXPR bool has_offset = (unit_a::offset::num != 0) || (unit_b::offset::num != 0);
};

template <typename A, typename B>
struct quotient
{
    using unit_a = typename quantity_like<A>::unit_type;
    using unit_b = typename quantity_like<B>::unit_type;
    using unit_type = unit<dimension_divide_t<typename unit_a::dimension, typename unit_b::dimension>,
                           math::ratio_divide_t<typename unit_a::ratio, typename unit_b::ratio>,
                           unit_a::pi_exponent - unit_b::pi_exponent>;
    using rep = meta::common_type_t<typename quantity_like<A>::rep, typename quantity_like<B>::rep>;
    using factory = quantity_factory<unit_type, rep>;
    using type = typename factory::type;
    static CASTLE_CONSTEXPR bool has_offset = (unit_a::offset::num != 0) || (unit_b::offset::num != 0);
};

template <typename Unit, typename Rep>
struct reciprocal
{
    using unit_type = unit<dimension_inverse_t<typename Unit::dimension>,
                           math::ratio_divide_t<math::ratio<1>, typename Unit::ratio>,
                           -Unit::pi_exponent>;
    using factory = quantity_factory<unit_type, Rep>;
    using type = typename factory::type;
};

/**
 * @brief Common quantity of two compatible quantities; has no `type` when incompatible.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2,
          bool Compatible = same_dimension_and_kind<Unit1, Unit2>::value>
struct common_quantity
{
};

template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
struct common_quantity<Unit1, Rep1, Unit2, Rep2, true>
{
private:
    // Same pi factor and origin: the exact common scale is gcd(num) / lcm(den), as in chrono.
    static CASTLE_CONSTEXPR bool keep_scale =
        (Unit1::pi_exponent == Unit2::pi_exponent) &&
        math::ratio_equal<typename Unit1::offset, typename Unit2::offset>::value;

    using scaled_unit = unit<typename Unit1::dimension,
                             math::ratio<math::gcd<Unit1::ratio::num, Unit2::ratio::num>::value,
                                         math::lcm<Unit1::ratio::den, Unit2::ratio::den>::value>,
                             Unit1::pi_exponent,
                             typename Unit1::offset,
                             typename Unit1::kind>;

public:
    using type = quantity<
        meta::conditional_t<keep_scale, scaled_unit, coherent_unit_t<Unit1>>,
        meta::conditional_t<keep_scale, meta::common_type_t<Rep1, Rep2>, floating_rep_t<Rep1, Rep2>>>;
};

template <typename Unit1, typename Unit2>
struct is_additive
    : meta::bool_constant<meta::is_same<Unit1, Unit2>::value ||
                          ((Unit1::offset::num == 0) && (Unit2::offset::num == 0))>
{
};

} // namespace detail

/**
 * @brief A value of arithmetic type `Rep` measured in the compile-time unit `Unit`.
 * @tparam Unit Unit built with `castle::units::unit`.
 * @tparam Rep Arithmetic representation type.
 * @note Pure time quantities are `castle::chrono::duration`; see `units/time.hpp`.
 */
template <typename Unit, typename Rep>
class quantity
{
public:
    using unit_type = Unit;                            ///< Unit of the stored value.
    using rep = Rep;                                   ///< Representation type.
    using dimension = typename Unit::dimension;        ///< Physical dimension.

    static_assert(detail::is_unit<Unit>::value,
                  "castle::units::quantity Unit must be built with castle::units::unit");
    static_assert(meta::is_arithmetic<Rep>::value,
                  "castle::units::quantity Rep must be an arithmetic type");
    static_assert(!(meta::is_same<dimension, dim::time>::value && meta::is_same<typename Unit::kind, void>::value),
                  "castle::units: time quantities are castle::chrono::duration (units/time.hpp)");

    /**
     * @brief Constructs a zero value.
     */
    CASTLE_CONSTEXPR quantity() CASTLE_NOEXCEPT
        : value_(static_cast<rep>(0))
    {
    }

    /**
     * @brief Constructs from a raw value already expressed in `Unit`.
     * @param value Raw value.
     */
    CASTLE_CONSTEXPR explicit quantity(rep value) CASTLE_NOEXCEPT
        : value_(value)
    {
    }

    /**
     * @brief Implicit lossless conversion from a compatible quantity.
     * @tparam Unit2 Source unit with the same dimension and kind.
     * @tparam Rep2 Source representation.
     * @param other Quantity to convert.
     */
    template <typename Unit2, typename Rep2,
              typename meta::enable_if<
                  detail::is_lossless_conversion<Unit2, Rep2, Unit, Rep>::value, int>::type = 0>
    CASTLE_CONSTEXPR quantity(CASTLE_CONST quantity<Unit2, Rep2>& other) CASTLE_NOEXCEPT
        : value_(detail::convert_value<Unit2, Unit, rep>(other.value()))
    {
    }

    /**
     * @brief Explicit lossy conversion from a compatible quantity.
     * @tparam Unit2 Source unit with the same dimension and kind.
     * @tparam Rep2 Source representation.
     * @param other Quantity to convert.
     * @note Integral results truncate toward zero.
     */
    template <typename Unit2, typename Rep2,
              typename meta::enable_if<
                  detail::same_dimension_and_kind<Unit2, Unit>::value &&
                  !detail::is_lossless_conversion<Unit2, Rep2, Unit, Rep>::value, int>::type = 0>
    CASTLE_CONSTEXPR explicit quantity(CASTLE_CONST quantity<Unit2, Rep2>& other) CASTLE_NOEXCEPT
        : value_(detail::convert_value<Unit2, Unit, rep>(other.value()))
    {
    }

    quantity(CASTLE_CONST quantity&) CASTLE_DEFAULT;
    quantity& operator=(CASTLE_CONST quantity&) CASTLE_DEFAULT;

    /**
     * @brief Returns the raw value expressed in `Unit`.
     * @return Stored value.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR rep value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value_;
    }

    /**
     * @brief Returns a zero quantity.
     * @return Quantity holding `Rep(0)`.
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR quantity zero() CASTLE_NOEXCEPT
    {
        return quantity(static_cast<rep>(0));
    }

    /**
     * @brief Returns the lowest representable quantity.
     * @return Quantity holding `numeric_limits<Rep>::lowest()`.
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR quantity min() CASTLE_NOEXCEPT
    {
        return quantity(castle::numeric_limits<rep>::lowest());
    }

    /**
     * @brief Returns the highest representable quantity.
     * @return Quantity holding `numeric_limits<Rep>::max()`.
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR quantity max() CASTLE_NOEXCEPT
    {
        return quantity(castle::numeric_limits<rep>::max());
    }

    /**
     * @brief Adds a quantity of the same type; lossless conversions apply implicitly.
     * @param other Quantity to add.
     * @return Reference to this quantity.
     */
    CASTLE_CONSTEXPR quantity& operator+=(CASTLE_CONST quantity& other) CASTLE_NOEXCEPT
    {
        value_ += other.value_;
        return *this;
    }

    /**
     * @brief Subtracts a quantity of the same type; lossless conversions apply implicitly.
     * @param other Quantity to subtract.
     * @return Reference to this quantity.
     */
    CASTLE_CONSTEXPR quantity& operator-=(CASTLE_CONST quantity& other) CASTLE_NOEXCEPT
    {
        value_ -= other.value_;
        return *this;
    }

    /**
     * @brief Multiplies by a dimensionless scalar.
     * @param scalar Multiplier.
     * @return Reference to this quantity.
     */
    CASTLE_CONSTEXPR quantity& operator*=(CASTLE_CONST rep& scalar) CASTLE_NOEXCEPT
    {
        value_ *= scalar;
        return *this;
    }

    /**
     * @brief Divides by a dimensionless scalar.
     * @param scalar Divisor.
     * @return Reference to this quantity.
     * @warning Division by zero is not checked.
     */
    CASTLE_CONSTEXPR quantity& operator/=(CASTLE_CONST rep& scalar) CASTLE_NOEXCEPT
    {
        value_ /= scalar;
        return *this;
    }

private:
    rep value_;
};

} // namespace units

namespace meta
{

/**
 * @brief Common type of two compatible quantities.
 * @note Undefined (no `type`) when dimensions or kinds differ.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
struct common_type<castle::units::quantity<Unit1, Rep1>, castle::units::quantity<Unit2, Rep2>>
    : castle::units::detail::common_quantity<Unit1, Rep1, Unit2, Rep2>
{
};

} // namespace meta

namespace units
{

/**
 * @brief Converts between quantities (or chrono durations) of the same dimension.
 * @tparam To Destination quantity or duration type.
 * @tparam From Source quantity or duration type.
 * @param from Value to convert.
 * @return `from` expressed as `To`; integral results truncate toward zero.
 * @note Kind tags may differ, e.g. a geographic angle can be cast to a plane angle.
 */
template <typename To, typename From>
CASTLE_NODISCARD CASTLE_CONSTEXPR To quantity_cast(CASTLE_CONST From& from) CASTLE_NOEXCEPT
{
    static_assert(detail::quantity_like<To>::value && detail::quantity_like<From>::value,
                  "castle::units::quantity_cast requires quantities or chrono durations");

    using from_traits = detail::quantity_like<From>;
    using to_traits = detail::quantity_like<To>;

    return To(detail::convert_value<typename from_traits::unit_type,
                                    typename to_traits::unit_type,
                                    typename to_traits::rep>(from_traits::value_of(from)));
}

/**
 * @brief Returns a quantity unchanged.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD CASTLE_CONSTEXPR quantity<Unit, Rep>
operator+(CASTLE_CONST quantity<Unit, Rep>& value) CASTLE_NOEXCEPT
{
    return value;
}

/**
 * @brief Negates a quantity.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD CASTLE_CONSTEXPR quantity<Unit, Rep>
operator-(CASTLE_CONST quantity<Unit, Rep>& value) CASTLE_NOEXCEPT
{
    return quantity<Unit, Rep>(static_cast<Rep>(-value.value()));
}

/**
 * @brief Adds two compatible quantities in their common unit.
 * @note Mixing two different offset units (e.g. Celsius and Fahrenheit) is rejected.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR typename detail::common_quantity<Unit1, Rep1, Unit2, Rep2>::type
operator+(CASTLE_CONST quantity<Unit1, Rep1>& lhs, CASTLE_CONST quantity<Unit2, Rep2>& rhs) CASTLE_NOEXCEPT
{
    static_assert(detail::is_additive<Unit1, Unit2>::value,
                  "castle::units: adding different offset units is ambiguous; convert one operand first");

    using result = typename detail::common_quantity<Unit1, Rep1, Unit2, Rep2>::type;
    using result_unit = typename result::unit_type;
    using result_rep = typename result::rep;

    return result(static_cast<result_rep>(
        detail::convert_value<Unit1, result_unit, result_rep>(lhs.value()) +
        detail::convert_value<Unit2, result_unit, result_rep>(rhs.value())));
}

/**
 * @brief Subtracts two compatible quantities in their common unit.
 * @note Mixing two different offset units (e.g. Celsius and Fahrenheit) is rejected.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR typename detail::common_quantity<Unit1, Rep1, Unit2, Rep2>::type
operator-(CASTLE_CONST quantity<Unit1, Rep1>& lhs, CASTLE_CONST quantity<Unit2, Rep2>& rhs) CASTLE_NOEXCEPT
{
    static_assert(detail::is_additive<Unit1, Unit2>::value,
                  "castle::units: subtracting different offset units is ambiguous; convert one operand first");

    using result = typename detail::common_quantity<Unit1, Rep1, Unit2, Rep2>::type;
    using result_unit = typename result::unit_type;
    using result_rep = typename result::rep;

    return result(static_cast<result_rep>(
        detail::convert_value<Unit1, result_unit, result_rep>(lhs.value()) -
        detail::convert_value<Unit2, result_unit, result_rep>(rhs.value())));
}

/**
 * @brief Multiplies a quantity by an arithmetic scalar.
 */
template <typename Unit, typename Rep, typename Scalar,
          typename meta::enable_if<meta::is_arithmetic<Scalar>::value, int>::type = 0>
CASTLE_NODISCARD CASTLE_CONSTEXPR quantity<Unit, meta::common_type_t<Rep, Scalar>>
operator*(CASTLE_CONST quantity<Unit, Rep>& value, CASTLE_CONST Scalar& scalar) CASTLE_NOEXCEPT
{
    using result_rep = meta::common_type_t<Rep, Scalar>;
    return quantity<Unit, result_rep>(static_cast<result_rep>(value.value()) * static_cast<result_rep>(scalar));
}

/**
 * @brief Multiplies an arithmetic scalar by a quantity.
 */
template <typename Scalar, typename Unit, typename Rep,
          typename meta::enable_if<meta::is_arithmetic<Scalar>::value, int>::type = 0>
CASTLE_NODISCARD CASTLE_CONSTEXPR quantity<Unit, meta::common_type_t<Rep, Scalar>>
operator*(CASTLE_CONST Scalar& scalar, CASTLE_CONST quantity<Unit, Rep>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

/**
 * @brief Divides a quantity by an arithmetic scalar.
 * @warning Division by zero is not checked.
 */
template <typename Unit, typename Rep, typename Scalar,
          typename meta::enable_if<meta::is_arithmetic<Scalar>::value, int>::type = 0>
CASTLE_NODISCARD CASTLE_CONSTEXPR quantity<Unit, meta::common_type_t<Rep, Scalar>>
operator/(CASTLE_CONST quantity<Unit, Rep>& value, CASTLE_CONST Scalar& scalar) CASTLE_NOEXCEPT
{
    using result_rep = meta::common_type_t<Rep, Scalar>;
    return quantity<Unit, result_rep>(static_cast<result_rep>(value.value()) / static_cast<result_rep>(scalar));
}

/**
 * @brief Divides an arithmetic scalar by a quantity, producing the reciprocal unit.
 * @note `1.0 / seconds_quantity` style results map to durations or plain values as usual.
 * @warning Division by zero is not checked.
 */
template <typename Scalar, typename Unit, typename Rep,
          typename meta::enable_if<meta::is_arithmetic<Scalar>::value, int>::type = 0>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename detail::reciprocal<Unit, meta::common_type_t<Rep, Scalar>>::type
operator/(CASTLE_CONST Scalar& scalar, CASTLE_CONST quantity<Unit, Rep>& value) CASTLE_NOEXCEPT
{
    static_assert(Unit::offset::num == 0,
                  "castle::units: the reciprocal of an offset unit is meaningless");

    using result_rep = meta::common_type_t<Rep, Scalar>;
    using recip = detail::reciprocal<Unit, result_rep>;
    return recip::factory::make(static_cast<result_rep>(scalar) / static_cast<result_rep>(value.value()));
}

/**
 * @brief Multiplies two quantities (or a quantity and a duration), deriving the result unit.
 * @return A quantity, a `chrono::duration` for pure time, or a plain value when dimensionless.
 */
template <typename A, typename B,
          typename meta::enable_if<detail::is_product_operands<A, B>::value, int>::type = 0>
CASTLE_NODISCARD CASTLE_CONSTEXPR typename detail::product<A, B>::type
operator*(CASTLE_CONST A& lhs, CASTLE_CONST B& rhs) CASTLE_NOEXCEPT
{
    using op = detail::product<A, B>;
    static_assert(!op::has_offset,
                  "castle::units: offset units (e.g. Celsius) cannot be multiplied; convert to the coherent unit first");

    using rep = typename op::rep;
    return op::factory::make(static_cast<rep>(detail::quantity_like<A>::value_of(lhs)) *
                             static_cast<rep>(detail::quantity_like<B>::value_of(rhs)));
}

/**
 * @brief Divides two quantities (or a quantity and a duration), deriving the result unit.
 * @return A quantity, a `chrono::duration` for pure time, or a plain value when dimensionless.
 * @warning Division by zero is not checked.
 */
template <typename A, typename B,
          typename meta::enable_if<detail::is_product_operands<A, B>::value, int>::type = 0>
CASTLE_NODISCARD CASTLE_CONSTEXPR typename detail::quotient<A, B>::type
operator/(CASTLE_CONST A& lhs, CASTLE_CONST B& rhs) CASTLE_NOEXCEPT
{
    using op = detail::quotient<A, B>;
    static_assert(!op::has_offset,
                  "castle::units: offset units (e.g. Celsius) cannot be divided; convert to the coherent unit first");

    using rep = typename op::rep;
    return op::factory::make(static_cast<rep>(detail::quantity_like<A>::value_of(lhs)) /
                             static_cast<rep>(detail::quantity_like<B>::value_of(rhs)));
}

/**
 * @brief Returns the reciprocal of a quantity or duration, e.g. the frequency of a period.
 * @tparam Q Quantity or `chrono::duration` type.
 * @param value Value to invert.
 * @return `1 / value` in the reciprocal unit.
 * @note Integral representations truncate; use a floating representation for periods.
 */
template <typename Q,
          typename meta::enable_if<detail::quantity_like<Q>::value, int>::type = 0>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename detail::reciprocal<typename detail::quantity_like<Q>::unit_type,
                            typename detail::quantity_like<Q>::rep>::type
inverse(CASTLE_CONST Q& value) CASTLE_NOEXCEPT
{
    using traits = detail::quantity_like<Q>;
    using unit_type = typename traits::unit_type;
    using rep = typename traits::rep;
    static_assert(unit_type::offset::num == 0,
                  "castle::units: the reciprocal of an offset unit is meaningless");

    return detail::reciprocal<unit_type, rep>::factory::make(
        static_cast<rep>(static_cast<rep>(1) / traits::value_of(value)));
}

/**
 * @brief Compares two compatible quantities for equality in their common unit.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<detail::same_dimension_and_kind<Unit1, Unit2>::value, bool>::type
operator==(CASTLE_CONST quantity<Unit1, Rep1>& lhs, CASTLE_CONST quantity<Unit2, Rep2>& rhs) CASTLE_NOEXCEPT
{
    using common = typename detail::common_quantity<Unit1, Rep1, Unit2, Rep2>::type;
    return common(lhs).value() == common(rhs).value();
}

/**
 * @brief Compares two compatible quantities for inequality.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<detail::same_dimension_and_kind<Unit1, Unit2>::value, bool>::type
operator!=(CASTLE_CONST quantity<Unit1, Rep1>& lhs, CASTLE_CONST quantity<Unit2, Rep2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Orders two compatible quantities in their common unit.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<detail::same_dimension_and_kind<Unit1, Unit2>::value, bool>::type
operator<(CASTLE_CONST quantity<Unit1, Rep1>& lhs, CASTLE_CONST quantity<Unit2, Rep2>& rhs) CASTLE_NOEXCEPT
{
    using common = typename detail::common_quantity<Unit1, Rep1, Unit2, Rep2>::type;
    return common(lhs).value() < common(rhs).value();
}

/**
 * @brief Orders two compatible quantities in their common unit.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<detail::same_dimension_and_kind<Unit1, Unit2>::value, bool>::type
operator<=(CASTLE_CONST quantity<Unit1, Rep1>& lhs, CASTLE_CONST quantity<Unit2, Rep2>& rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

/**
 * @brief Orders two compatible quantities in their common unit.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<detail::same_dimension_and_kind<Unit1, Unit2>::value, bool>::type
operator>(CASTLE_CONST quantity<Unit1, Rep1>& lhs, CASTLE_CONST quantity<Unit2, Rep2>& rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

/**
 * @brief Orders two compatible quantities in their common unit.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<detail::same_dimension_and_kind<Unit1, Unit2>::value, bool>::type
operator>=(CASTLE_CONST quantity<Unit1, Rep1>& lhs, CASTLE_CONST quantity<Unit2, Rep2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

/**
 * @brief Returns the magnitude of a quantity in the same unit.
 * @warning For offset units the result is relative to the unit's own zero.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD CASTLE_CONSTEXPR quantity<Unit, Rep>
abs(CASTLE_CONST quantity<Unit, Rep>& value) CASTLE_NOEXCEPT
{
    return (value.value() < static_cast<Rep>(0)) ? -value : value;
}

} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_QUANTITY_HPP
