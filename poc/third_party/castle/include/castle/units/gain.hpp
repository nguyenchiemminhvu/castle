// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file gain.hpp
 * @brief Logarithmic gain units: the bel and the decibel, with conversions to and from linear ratios.
 *
 * Gain is dimensionless, but it carries the `gain_kind` tag. A gain therefore
 * never collapses to a plain number and cannot be mixed with a scalar by
 * accident. Adding two gains in decibels cascades two stages.
 *
 * Example:
 * @code
 * #include "castle/units/gain.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::decibels<double> total = 20.0_dB + 3.0_dB;     // cascaded stages: 23 dB
 * const double p = castle::units::power_ratio(3.0_dB);           // about 2.0
 * const double a = castle::units::amplitude_ratio(20.0_dB);      // 10.0
 * auto g = castle::units::decibels_from_power_ratio(100.0);      // decibels<double>: 20 dB
 * @endcode
 *
 * Key constraints:
 * - `power_ratio` and `amplitude_ratio` return `10^(dB/10)` and `10^(dB/20)`;
 *   `decibels_from_power_ratio` and `decibels_from_amplitude_ratio` need a
 *   strictly positive ratio (the logarithm of zero or a negative value is not
 *   defined).
 * - Absolute levels: dBm is in power.hpp (`decibel_milliwatts`); dBV is out of scope.
 * - Math functions wrap the platform C math library (`pow`, `log10`).
 */
#ifndef CASTLE_UNITS_GAIN_HPP
#define CASTLE_UNITS_GAIN_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/scale.hpp"

#include <math.h>
#include <stdint.h>

namespace castle
{
namespace units
{

/// @brief Kind tag of logarithmic gain; keeps gain apart from plain dimensionless numbers.
struct gain_kind
{
};

/// @brief Bel, the coherent logarithmic gain unit (a factor of ten in power).
using bel_unit = unit<dim::dimensionless, math::ratio<1>, 0, math::ratio<0>, gain_kind>;
/// @brief Decibel, 1/10 bel.
using decibel_unit = scaled_unit_t<bel_unit, deci>;

/// @brief Gain in bels.
template <typename Rep>
using bels = quantity<bel_unit, Rep>;
/// @brief Gain in decibels.
template <typename Rep>
using decibels = quantity<decibel_unit, Rep>;

namespace detail
{

/**
 * @brief Gain traits: enabled for dimensionless units of `gain_kind`, with the floating result type.
 */
template <typename Unit, typename Rep>
struct gain_traits
{
    static CASTLE_CONSTEXPR bool value =
        meta::is_same<typename Unit::dimension, dim::dimensionless>::value &&
        meta::is_same<typename Unit::kind, gain_kind>::value;
    using real = floating_rep_t<Rep, Rep>;
};

template <typename Unit, typename Rep>
using enable_if_gain_t = typename meta::enable_if<gain_traits<Unit, Rep>::value,
                                                  typename gain_traits<Unit, Rep>::real>::type;

CASTLE_INLINE float pow(float x, float y) CASTLE_NOEXCEPT
{
    return ::powf(x, y);
}

CASTLE_INLINE double pow(double x, double y) CASTLE_NOEXCEPT
{
    return ::pow(x, y);
}

CASTLE_INLINE long double pow(long double x, long double y) CASTLE_NOEXCEPT
{
    return ::powl(x, y);
}

CASTLE_INLINE float log10(float x) CASTLE_NOEXCEPT
{
    return ::log10f(x);
}

CASTLE_INLINE double log10(double x) CASTLE_NOEXCEPT
{
    return ::log10(x);
}

CASTLE_INLINE long double log10(long double x) CASTLE_NOEXCEPT
{
    return ::log10l(x);
}

/**
 * @brief Gain value in decibels, in the floating type of `Rep`.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD CASTLE_CONSTEXPR typename gain_traits<Unit, Rep>::real
decibel_value(CASTLE_CONST quantity<Unit, Rep>& gain) CASTLE_NOEXCEPT
{
    return convert_value<Unit, decibel_unit, typename gain_traits<Unit, Rep>::real>(gain.value());
}

} // namespace detail

/**
 * @brief Linear power ratio of a gain.
 * @param gain Gain in any gain-kind unit (bels, decibels, ...).
 * @return `10^(dB / 10)`; floating type of `Rep`, or `double` for integral reps.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD detail::enable_if_gain_t<Unit, Rep>
power_ratio(CASTLE_CONST quantity<Unit, Rep>& gain) CASTLE_NOEXCEPT
{
    using real = typename detail::gain_traits<Unit, Rep>::real;
    return detail::pow(static_cast<real>(10), detail::decibel_value(gain) / static_cast<real>(10));
}

/**
 * @brief Linear amplitude (voltage) ratio of a gain.
 * @param gain Gain in any gain-kind unit (bels, decibels, ...).
 * @return `10^(dB / 20)`; floating type of `Rep`, or `double` for integral reps.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD detail::enable_if_gain_t<Unit, Rep>
amplitude_ratio(CASTLE_CONST quantity<Unit, Rep>& gain) CASTLE_NOEXCEPT
{
    using real = typename detail::gain_traits<Unit, Rep>::real;
    return detail::pow(static_cast<real>(10), detail::decibel_value(gain) / static_cast<real>(20));
}

/**
 * @brief Gain of a linear power ratio.
 * @tparam T Floating-point type.
 * @param ratio Output power divided by input power; must be positive.
 * @return `10 * log10(ratio)` decibels.
 */
template <typename T>
CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<T>::value, decibels<T>>::type
decibels_from_power_ratio(T ratio) CASTLE_NOEXCEPT
{
    return decibels<T>(static_cast<T>(static_cast<T>(10) * detail::log10(ratio)));
}

/**
 * @brief Gain of a linear amplitude (voltage) ratio.
 * @tparam T Floating-point type.
 * @param ratio Output amplitude divided by input amplitude; must be positive.
 * @return `20 * log10(ratio)` decibels.
 */
template <typename T>
CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<T>::value, decibels<T>>::type
decibels_from_amplitude_ratio(T ratio) CASTLE_NOEXCEPT
{
    return decibels<T>(static_cast<T>(static_cast<T>(20) * detail::log10(ratio)));
}

namespace literals
{

/// @brief Floating decibels literal, e.g. `3.0_dB`.
CASTLE_CONSTEXPR decibels<double> operator""_dB(long double value) CASTLE_NOEXCEPT
{
    return decibels<double>(static_cast<double>(value));
}

/// @brief Integer decibels literal, e.g. `20_dB`.
CASTLE_CONSTEXPR decibels<int64_t> operator""_dB(unsigned long long value) CASTLE_NOEXCEPT
{
    return decibels<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating bels literal, e.g. `1.5_B`.
CASTLE_CONSTEXPR bels<double> operator""_B(long double value) CASTLE_NOEXCEPT
{
    return bels<double>(static_cast<double>(value));
}

/// @brief Integer bels literal, e.g. `2_B`.
CASTLE_CONSTEXPR bels<int64_t> operator""_B(unsigned long long value) CASTLE_NOEXCEPT
{
    return bels<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_GAIN_HPP
