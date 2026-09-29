// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file zephyr.hpp
 * @brief Zephyr GPIO backend for `gpio_dt_spec` handles.
 *
 * The adapter uses Zephyr's logical operations (`gpio_pin_set_dt` /
 * `gpio_pin_get_dt`) for the portable `write`/`read` calls, while the raw
 * operations use `gpio_pin_set_raw` / `gpio_pin_get_raw` so active-low device
 * tree flags are not applied twice.
 */
#ifndef CASTLE_EXT_GPIO_BACKENDS_ZEPHYR_HPP
#define CASTLE_EXT_GPIO_BACKENDS_ZEPHYR_HPP

#include "castle/error/status.hpp"
#include "castle_ext/gpio/types.hpp"

#if defined(CASTLE_EXT_ENABLE_GPIO_ZEPHYR)
    #include <zephyr/drivers/gpio.h>
#endif

namespace castle
{
namespace gpio
{
namespace zephyr
{

#if defined(CASTLE_EXT_ENABLE_GPIO_ZEPHYR)

struct backend
{
    using native_handle_type = gpio_dt_spec;

    static castle::status configure(native_handle_type const& handle, pin_config const& config) CASTLE_NOEXCEPT
    {
        if (!gpio_is_ready_dt(&handle))
        {
            return castle::status::not_configured;
        }

        uint32_t flags = 0U;

        if (config.dir == direction::input)
        {
            flags |= GPIO_INPUT;
        }
        else
        {
            flags |= GPIO_OUTPUT;
            if (config.initial_valid)
            {
                flags |= (config.initial == level::high)
                    ? GPIO_OUTPUT_ACTIVE
                    : GPIO_OUTPUT_INACTIVE;
            }
        }

        switch (config.pull_mode)
        {
            case pull::none:
                break;
            case pull::up:
                flags |= GPIO_PULL_UP;
                break;
            case pull::down:
                flags |= GPIO_PULL_DOWN;
                break;
            default:
                return castle::status::invalid_config;
        }

        switch (config.drive_mode)
        {
            case drive::push_pull:
                break;
            case drive::open_drain:
                flags |= GPIO_OPEN_DRAIN;
                break;
            case drive::open_source:
                flags |= GPIO_OPEN_SOURCE;
                break;
            default:
                return castle::status::invalid_config;
        }

        return gpio_pin_configure_dt(&handle, flags) == 0
            ? castle::status::ok
            : castle::status::system_call_error;
    }

    static castle::status write(native_handle_type const& handle, level value) CASTLE_NOEXCEPT
    {
        if (!gpio_is_ready_dt(&handle))
        {
            return castle::status::not_configured;
        }
        return gpio_pin_set_dt(&handle, to_bool(value)) == 0
            ? castle::status::ok
            : castle::status::system_call_error;
    }

    static castle::status read(native_handle_type const& handle, level& out_value) CASTLE_NOEXCEPT
    {
        if (!gpio_is_ready_dt(&handle))
        {
            return castle::status::not_configured;
        }
        const int result = gpio_pin_get_dt(&handle);
        if (result < 0)
        {
            return castle::status::system_call_error;
        }
        out_value = result != 0 ? level::high : level::low;
        return castle::status::ok;
    }

    static castle::status write_raw(native_handle_type const& handle, level value) CASTLE_NOEXCEPT
    {
        if (!gpio_is_ready_dt(&handle))
        {
            return castle::status::not_configured;
        }
        return gpio_pin_set_raw(handle.port, handle.pin, to_bool(value)) == 0
            ? castle::status::ok
            : castle::status::system_call_error;
    }

    static castle::status read_raw(native_handle_type const& handle, level& out_value) CASTLE_NOEXCEPT
    {
        if (!gpio_is_ready_dt(&handle))
        {
            return castle::status::not_configured;
        }
        const int result = gpio_pin_get_raw(handle.port, handle.pin);
        if (result < 0)
        {
            return castle::status::system_call_error;
        }
        out_value = result != 0 ? level::high : level::low;
        return castle::status::ok;
    }

    static castle::status toggle(native_handle_type const& handle) CASTLE_NOEXCEPT
    {
        if (!gpio_is_ready_dt(&handle))
        {
            return castle::status::not_configured;
        }
        return gpio_pin_toggle_dt(&handle) == 0
            ? castle::status::ok
            : castle::status::system_call_error;
    }
};

#else

/**
 * @brief Placeholder declaration when Zephyr support is not enabled.
 *
 * Define `CASTLE_EXT_ENABLE_GPIO_ZEPHYR` in a Zephyr build before including
 * this header. Keeping the header parseable on host builds makes umbrella
 * includes harmless for tools and unit tests.
 */
struct backend;

#endif

} // namespace zephyr
} // namespace gpio
} // namespace castle

#endif // CASTLE_EXT_GPIO_BACKENDS_ZEPHYR_HPP
