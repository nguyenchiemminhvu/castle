// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mock.hpp
 * @brief Deterministic allocation-free GPIO backend for host-side tests.
 */
#ifndef CASTLE_EXT_GPIO_BACKENDS_MOCK_HPP
#define CASTLE_EXT_GPIO_BACKENDS_MOCK_HPP

#include "castle/error/status.hpp"
#include "castle_ext/gpio/types.hpp"

namespace castle
{
namespace gpio
{
namespace mock
{

struct pin_state
{
    level physical;
    pin_config config;
    bool configured;
    bool active_low;

    CASTLE_CONSTEXPR pin_state(
        bool invert = false,
        level initial = level::low) CASTLE_NOEXCEPT
        : physical(initial)
        , config()
        , configured(false)
        , active_low(invert)
    {
    }
};

struct backend
{
    using native_handle_type = pin_state*;

    static castle::status configure(native_handle_type handle, pin_config const& config) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        handle->config = config;
        handle->configured = true;
        return castle::status::ok;
    }

    static castle::status write(native_handle_type handle, level value) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        handle->physical = handle->active_low ? invert(value) : value;
        return castle::status::ok;
    }

    static castle::status read(native_handle_type handle, level& out_value) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        out_value = handle->active_low ? invert(handle->physical) : handle->physical;
        return castle::status::ok;
    }

    static castle::status write_raw(native_handle_type handle, level value) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        handle->physical = value;
        return castle::status::ok;
    }

    static castle::status read_raw(native_handle_type handle, level& out_value) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        out_value = handle->physical;
        return castle::status::ok;
    }

    static castle::status toggle(native_handle_type handle) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        handle->physical = gpio::invert(handle->physical);
        return castle::status::ok;
    }
};

/** @brief Deterministic in-memory state backing `port_backend`. */
struct port_state
{
    mask_type physical;
    mask_type active_low_mask;

    explicit CASTLE_CONSTEXPR port_state(
        mask_type initial = 0U,
        mask_type invert_mask = 0U) CASTLE_NOEXCEPT
        : physical(initial)
        , active_low_mask(invert_mask)
    {
    }
};

/** @brief Deterministic allocation-free GPIO port backend for host-side tests. */
struct port_backend
{
    using native_handle_type = port_state*;

    static castle::status read(native_handle_type handle, mask_type& out_value) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        out_value = handle->physical ^ handle->active_low_mask;
        return castle::status::ok;
    }

    static castle::status read_raw(native_handle_type handle, mask_type& out_value) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        out_value = handle->physical;
        return castle::status::ok;
    }

    static castle::status write(native_handle_type handle, mask_type value) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        handle->physical = value ^ handle->active_low_mask;
        return castle::status::ok;
    }

    static castle::status write_masked(native_handle_type handle, mask_type mask, mask_type value) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        const mask_type physical_value = value ^ handle->active_low_mask;
        handle->physical = (handle->physical & ~mask) | (physical_value & mask);
        return castle::status::ok;
    }

    static castle::status set(native_handle_type handle, mask_type mask) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        return write_masked(handle, mask, mask);
    }

    static castle::status clear(native_handle_type handle, mask_type mask) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        return write_masked(handle, mask, 0U);
    }

    static castle::status toggle(native_handle_type handle, mask_type mask) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (handle == nullptr)
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        handle->physical ^= mask;
        return castle::status::ok;
    }
};

} // namespace mock
} // namespace gpio
} // namespace castle

#endif // CASTLE_EXT_GPIO_BACKENDS_MOCK_HPP
