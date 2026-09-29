// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file linux_file.hpp
 * @brief Allocation-free Linux/Unix file-backed GPIO adapter.
 *
 * This adapter is intended for BSPs that expose one GPIO as a user-space value
 * file. It uses POSIX file descriptors directly and deliberately does not pull
 * in `<string>`, streams, or filesystem classes.
 *
 * Direction/pull/drive configuration is treated as BSP-owned configuration;
 * `configure()` therefore returns `status::not_configured` rather than
 * pretending a generic value file can change electrical configuration.
 */
#ifndef CASTLE_EXT_IO_GPIO_BACKENDS_LINUX_VALUE_FILE_HPP
#define CASTLE_EXT_IO_GPIO_BACKENDS_LINUX_VALUE_FILE_HPP

#include "castle/error/status.hpp"
#include "castle_ext/io/gpio/types.hpp"

#if defined(CASTLE_EXT_ENABLE_GPIO_LINUX_FILE)
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace castle
{
namespace io
{
namespace gpio
{
namespace linux_file
{

struct pin_handle
{
    CASTLE_CONST char* path;
    bool active_low;

    CASTLE_CONSTEXPR pin_handle(CASTLE_CONST char* p = nullptr, bool invert = false) CASTLE_NOEXCEPT
        : path(p)
        , active_low(invert)
    {
    }
};

#if defined(CASTLE_EXT_ENABLE_GPIO_LINUX_FILE)

struct backend
{
    using native_handle_type = pin_handle;

    static castle::status configure(native_handle_type CASTLE_CONST&, pin_config CASTLE_CONST&) CASTLE_NOEXCEPT
    {
        return castle::status::not_configured;
    }

    static castle::status write(native_handle_type CASTLE_CONST& handle, level value) CASTLE_NOEXCEPT
    {
        return write_impl(handle, handle.active_low ? gpio::invert(value) : value);
    }

    static castle::status read(native_handle_type CASTLE_CONST& handle, level& out_value) CASTLE_NOEXCEPT
    {
        level physical = level::low;
        CASTLE_CONST castle::status result = read_impl(handle, physical);
        if (!castle::succeeded(result))
        {
            return result;
        }
        out_value = handle.active_low ? gpio::invert(physical) : physical; // LCOV_EXCL_BR_LINE
        return castle::status::ok;
    }

    static castle::status write_raw(native_handle_type CASTLE_CONST& handle, level value) CASTLE_NOEXCEPT
    {
        return write_impl(handle, value);
    }

    static castle::status read_raw(native_handle_type CASTLE_CONST& handle, level& out_value) CASTLE_NOEXCEPT
    {
        return read_impl(handle, out_value);
    }

    static castle::status toggle(native_handle_type CASTLE_CONST& handle) CASTLE_NOEXCEPT
    {
        level current = level::low;
        CASTLE_CONST castle::status result = read(handle, current);
        if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            return result;
        }
        return write(handle, current == level::high ? level::low : level::high);
    }

private:
    static castle::status write_impl(native_handle_type CASTLE_CONST& handle, level value) CASTLE_NOEXCEPT
    {
        if (handle.path == nullptr)
        {
            return castle::status::invalid_argument;
        }

        CASTLE_CONST int fd = open(handle.path, O_WRONLY);
        if (fd < 0)
        {
            return castle::status::system_call_error;
        }

        CASTLE_CONST char c = value == level::high ? '1' : '0';
        CASTLE_CONST ssize_t count = ::write(fd, &c, 1);
        CASTLE_CONST int close_result = close(fd);

        return (count == 1 && close_result == 0) ? castle::status::ok : castle::status::system_call_error; // LCOV_EXCL_BR_LINE
    }

    static castle::status read_impl(native_handle_type CASTLE_CONST& handle, level& out_value) CASTLE_NOEXCEPT
    {
        if (handle.path == nullptr)
        {
            return castle::status::invalid_argument;
        }

        CASTLE_CONST int fd = open(handle.path, O_RDONLY);
        if (fd < 0)
        {
            return castle::status::system_call_error;
        }

        char buffer[4] = {0, 0, 0, 0};
        CASTLE_CONST ssize_t count = ::read(fd, buffer, sizeof(buffer));
        CASTLE_CONST int close_result = close(fd);

        // LCOV_EXCL_START
        if (close_result != 0)
        {
            return castle::status::system_call_error;
        }
        if (count <= 0)
        {
            return castle::status::system_call_error;
        }
        // LCOV_EXCL_STOP

        for (ssize_t i = 0; i < count; ++i) // LCOV_EXCL_BR_LINE
        {
            if (buffer[i] == '0')
            {
                out_value = level::low;
                return castle::status::ok;
            }
            if (buffer[i] == '1') // LCOV_EXCL_BR_LINE
            {
                out_value = level::high;
                return castle::status::ok;
            }
        }

        return castle::status::invalid_argument; // LCOV_EXCL_LINE
    }
};

#else

struct backend;

#endif

} // namespace linux_file
} // namespace gpio
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_GPIO_BACKENDS_LINUX_VALUE_FILE_HPP
