// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file power.hpp
 * @brief Power units: the watt with SI multiples, and the dBm absolute power level.
 *
 * Example:
 * @code
 * #include "castle/units/power.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::watts<double> budget = 1.5_W + 250.0_mW;   // 1.75 W
 * castle::units::microwatts<int64_t> sleep = 3_mW;          // implicit, exact: 3000 uW
 *
 * castle::units::watts<double> tx = castle::units::milliwatts_from_dbm(30.0_dBm);   // 1 W
 * auto level = castle::units::dbm_from_power(1.0_W);                               // 30 dBm
 * auto rx = 30.0_dBm - 3.0_dB;                                                     // 27 dBm
 * @endcode
 *
 * Key constraints:
 * - `dim::power` is L^2 M T^-3; energy divided by a `chrono::duration` in
 *   coherent units is exactly `watts<Rep>`, and watts times hours is
 *   exactly `watt_hours<Rep>` from energy.hpp.
 * - dBm is logarithmic (`P[dBm] = 10 * log10(P / 1 mW)`), so it is a separate
 *   `power_level_kind` quantity, not a scaled watt. It leaves and enters the
 *   linear domain only through `milliwatts_from_dbm` and `dbm_from_power`.
 *   A level plus or minus a gain is a level; a level minus a level is a gain;
 *   adding two levels is rejected.
 * - `dbm_from_power` needs a strictly positive power; the logarithm is not checked.
 */
#ifndef CASTLE_UNITS_POWER_HPP
#define CASTLE_UNITS_POWER_HPP

#include "castle/core/compiler.hpp"
#include "castle/units/gain.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/scale.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{

namespace dim
{
/// @brief Power, L^2 M T^-3.
using power = dimension<2, 1, -3, 0, 0, 0, 0, 0>;
} // namespace dim

/// @brief Watt, the coherent power unit (J/s).
using watt_unit = unit<dim::power>;
/// @brief Microwatt.
using microwatt_unit = scaled_unit_t<watt_unit, micro>;
/// @brief Milliwatt.
using milliwatt_unit = scaled_unit_t<watt_unit, milli>;
/// @brief Kilowatt.
using kilowatt_unit = scaled_unit_t<watt_unit, kilo>;
/// @brief Megawatt.
using megawatt_unit = scaled_unit_t<watt_unit, mega>;

/// @brief Power in watts.
template <typename Rep>
using watts = quantity<watt_unit, Rep>;
/// @brief Power in microwatts.
template <typename Rep>
using microwatts = quantity<microwatt_unit, Rep>;
/// @brief Power in milliwatts.
template <typename Rep>
using milliwatts = quantity<milliwatt_unit, Rep>;
/// @brief Power in kilowatts.
template <typename Rep>
using kilowatts = quantity<kilowatt_unit, Rep>;
/// @brief Power in megawatts.
template <typename Rep>
using megawatts = quantity<megawatt_unit, Rep>;

/// @brief Kind tag of absolute power levels in decibels; keeps dBm apart from relative gain.
struct power_level_kind
{
};

/// @brief Decibel-milliwatt (dBm), absolute power level referenced to 1 mW.
using decibel_milliwatt_unit = unit<dim::dimensionless, math::ratio<1>, 0, math::ratio<0>, power_level_kind>;

/// @brief Absolute power level in dBm.
template <typename Rep>
using decibel_milliwatts = quantity<decibel_milliwatt_unit, Rep>;

/**
 * @brief Linear power of a dBm level.
 * @param level Power level in dBm.
 * @return `10^(dBm / 10)` milliwatts; floating type of `Rep`, or `double` for integral reps.
 */
template <typename Rep>
CASTLE_NODISCARD milliwatts<detail::floating_rep_t<Rep, Rep>>
milliwatts_from_dbm(CASTLE_CONST decibel_milliwatts<Rep>& level) CASTLE_NOEXCEPT
{
    using real = detail::floating_rep_t<Rep, Rep>;
    return milliwatts<real>(
        detail::pow(static_cast<real>(10), static_cast<real>(level.value()) / static_cast<real>(10)));
}

/**
 * @brief dBm level of a linear power.
 * @param power Power in any power unit (watts, milliwatts, ...); must be positive.
 * @return `10 * log10(P / 1 mW)` dBm; floating type of `Rep`, or `double` for integral reps.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD typename meta::enable_if<detail::same_dimension_and_kind<Unit, watt_unit>::value,
                                          decibel_milliwatts<detail::floating_rep_t<Rep, Rep>>>::type
dbm_from_power(CASTLE_CONST quantity<Unit, Rep>& power) CASTLE_NOEXCEPT
{
    using real = detail::floating_rep_t<Rep, Rep>;
    return decibel_milliwatts<real>(static_cast<real>(
        static_cast<real>(10) * detail::log10(detail::convert_value<Unit, milliwatt_unit, real>(power.value()))));
}

/**
 * @brief Level raised by a gain, e.g. `10.0_dBm + 20.0_dB` is 30 dBm.
 * @param level Power level in dBm.
 * @param gain Gain in any gain-kind unit (decibels, bels).
 */
template <typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<detail::gain_traits<Unit2, Rep2>::value, decibel_milliwatts<meta::common_type_t<Rep1, Rep2>>>::type
operator+(CASTLE_CONST decibel_milliwatts<Rep1>& level, CASTLE_CONST quantity<Unit2, Rep2>& gain) CASTLE_NOEXCEPT
{
    using rep = meta::common_type_t<Rep1, Rep2>;
    return decibel_milliwatts<rep>(static_cast<rep>(
        static_cast<rep>(level.value()) + detail::convert_value<Unit2, decibel_unit, rep>(gain.value())));
}

/**
 * @brief Level raised by a gain, e.g. `20.0_dB + 10.0_dBm` is 30 dBm.
 * @param gain Gain in any gain-kind unit (decibels, bels).
 * @param level Power level in dBm.
 */
template <typename Unit1, typename Rep1, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<detail::gain_traits<Unit1, Rep1>::value, decibel_milliwatts<meta::common_type_t<Rep1, Rep2>>>::type
operator+(CASTLE_CONST quantity<Unit1, Rep1>& gain, CASTLE_CONST decibel_milliwatts<Rep2>& level) CASTLE_NOEXCEPT
{
    return level + gain;
}

/**
 * @brief Level lowered by a loss, e.g. `30.0_dBm - 3.0_dB` is 27 dBm.
 * @param level Power level in dBm.
 * @param loss Gain in any gain-kind unit (decibels, bels).
 */
template <typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<detail::gain_traits<Unit2, Rep2>::value, decibel_milliwatts<meta::common_type_t<Rep1, Rep2>>>::type
operator-(CASTLE_CONST decibel_milliwatts<Rep1>& level, CASTLE_CONST quantity<Unit2, Rep2>& loss) CASTLE_NOEXCEPT
{
    using rep = meta::common_type_t<Rep1, Rep2>;
    return decibel_milliwatts<rep>(static_cast<rep>(
        static_cast<rep>(level.value()) - detail::convert_value<Unit2, decibel_unit, rep>(loss.value())));
}

/**
 * @brief Gain between two levels, e.g. `30.0_dBm - 10.0_dBm` is 20 dB.
 * @param lhs Power level in dBm.
 * @param rhs Power level in dBm.
 */
template <typename Rep1, typename Rep2>
CASTLE_NODISCARD CASTLE_CONSTEXPR decibels<meta::common_type_t<Rep1, Rep2>>
operator-(CASTLE_CONST decibel_milliwatts<Rep1>& lhs, CASTLE_CONST decibel_milliwatts<Rep2>& rhs) CASTLE_NOEXCEPT
{
    using rep = meta::common_type_t<Rep1, Rep2>;
    return decibels<rep>(static_cast<rep>(static_cast<rep>(lhs.value()) - static_cast<rep>(rhs.value())));
}

/// @brief Adding two absolute levels has no physical meaning; sum the linear powers instead.
template <typename Rep1, typename Rep2>
void operator+(CASTLE_CONST decibel_milliwatts<Rep1>& lhs, CASTLE_CONST decibel_milliwatts<Rep2>& rhs) = delete;

namespace literals
{

/// @brief Floating microwatts literal, e.g. `1.5_uW`.
CASTLE_CONSTEXPR microwatts<double> operator""_uW(long double value) CASTLE_NOEXCEPT
{
    return microwatts<double>(static_cast<double>(value));
}

/// @brief Integer microwatts literal, e.g. `15_uW`.
CASTLE_CONSTEXPR microwatts<int64_t> operator""_uW(unsigned long long value) CASTLE_NOEXCEPT
{
    return microwatts<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating milliwatts literal, e.g. `1.5_mW`.
CASTLE_CONSTEXPR milliwatts<double> operator""_mW(long double value) CASTLE_NOEXCEPT
{
    return milliwatts<double>(static_cast<double>(value));
}

/// @brief Integer milliwatts literal, e.g. `15_mW`.
CASTLE_CONSTEXPR milliwatts<int64_t> operator""_mW(unsigned long long value) CASTLE_NOEXCEPT
{
    return milliwatts<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating watts literal, e.g. `1.5_W`.
CASTLE_CONSTEXPR watts<double> operator""_W(long double value) CASTLE_NOEXCEPT
{
    return watts<double>(static_cast<double>(value));
}

/// @brief Integer watts literal, e.g. `15_W`.
CASTLE_CONSTEXPR watts<int64_t> operator""_W(unsigned long long value) CASTLE_NOEXCEPT
{
    return watts<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kilowatts literal, e.g. `1.5_kW`.
CASTLE_CONSTEXPR kilowatts<double> operator""_kW(long double value) CASTLE_NOEXCEPT
{
    return kilowatts<double>(static_cast<double>(value));
}

/// @brief Integer kilowatts literal, e.g. `15_kW`.
CASTLE_CONSTEXPR kilowatts<int64_t> operator""_kW(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilowatts<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating megawatts literal, e.g. `1.5_MW`.
CASTLE_CONSTEXPR megawatts<double> operator""_MW(long double value) CASTLE_NOEXCEPT
{
    return megawatts<double>(static_cast<double>(value));
}

/// @brief Integer megawatts literal, e.g. `15_MW`.
CASTLE_CONSTEXPR megawatts<int64_t> operator""_MW(unsigned long long value) CASTLE_NOEXCEPT
{
    return megawatts<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating dBm literal, e.g. `30.0_dBm`.
CASTLE_CONSTEXPR decibel_milliwatts<double> operator""_dBm(long double value) CASTLE_NOEXCEPT
{
    return decibel_milliwatts<double>(static_cast<double>(value));
}

/// @brief Integer dBm literal, e.g. `30_dBm`.
CASTLE_CONSTEXPR decibel_milliwatts<int64_t> operator""_dBm(unsigned long long value) CASTLE_NOEXCEPT
{
    return decibel_milliwatts<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_POWER_HPP
