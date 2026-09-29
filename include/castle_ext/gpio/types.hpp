// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file types.hpp
 * @brief Platform-neutral GPIO value and configuration types.
 */
#ifndef CASTLE_EXT_GPIO_TYPES_HPP
#define CASTLE_EXT_GPIO_TYPES_HPP

#include "castle/core/compiler.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace gpio
{

/**
 * @brief Electrical/logical pin level.
 *
 * `low` and `high` are deliberately independent of active-low polarity.
 * Backend adapters expose both logical and raw operations when the platform
 * distinguishes them.
 */
enum class level : uint8_t
{
    low  = 0U,
    high = 1U
};

/** @brief GPIO direction. */
enum class direction : uint8_t
{
    input = 0U,
    output
};

/** @brief Internal pull configuration. */
enum class pull : uint8_t
{
    none = 0U,
    up,
    down
};

/** @brief GPIO output driver mode. */
enum class drive : uint8_t
{
    push_pull = 0U,
    open_drain,
    open_source
};

/**
 * @brief Fixed-size pin configuration.
 *
 * The `initial_valid` flag makes output initialization deterministic without
 * forcing a third sentinel state into the `level` enum.
 */
struct pin_config
{
    direction dir;
    level initial;
    pull pull_mode;
    drive drive_mode;
    bool initial_valid;

    CASTLE_CONSTEXPR pin_config(
        direction d = direction::input,
        level initial_level = level::low,
        pull p = pull::none,
        drive dr = drive::push_pull,
        bool has_initial = false) CASTLE_NOEXCEPT
        : dir(d)
        , initial(initial_level)
        , pull_mode(p)
        , drive_mode(dr)
        , initial_valid(has_initial)
    {
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_input() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return dir == direction::input;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_output() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return dir == direction::output;
    }
};

/** @brief Return true when a level is high. */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_high(level value) CASTLE_NOEXCEPT
{
    return value == level::high;
}

/** @brief Return true when a level is low. */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_low(level value) CASTLE_NOEXCEPT
{
    return value == level::low;
}

/** @brief Convert a boolean to a GPIO level. */
CASTLE_NODISCARD CASTLE_CONSTEXPR level to_level(bool value) CASTLE_NOEXCEPT
{
    return value ? level::high : level::low;
}

/** @brief Convert a GPIO level to a boolean. */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool to_bool(level value) CASTLE_NOEXCEPT
{
    return value == level::high;
}

/** @brief Return the opposite level, used by active-low backends to flip between physical and logical state. */
CASTLE_NODISCARD CASTLE_CONSTEXPR level invert(level value) CASTLE_NOEXCEPT
{
    return value == level::high ? level::low : level::high;
}

/** @brief Bit mask type for fixed-width GPIO port backends. */
using mask_type = uint32_t;

} // namespace gpio
} // namespace castle

#endif // CASTLE_EXT_GPIO_TYPES_HPP
