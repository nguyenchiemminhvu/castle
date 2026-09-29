// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file uart.hpp
 * @brief Allocation-free Linux/POSIX UART configuration and byte-stream access.
 *
 * A UART owns one descriptor and applies a raw `termios` configuration after
 * opening its device node. The configuration is a POD-style structure with a
 * borrowed path; it has no dependency on the legacy helper's STL or exception
 * abstractions. Reads and writes expose partial byte counts and never spin.
 */
#ifndef CASTLE_EXT_IO_UART_UART_HPP
#define CASTLE_EXT_IO_UART_UART_HPP

#include "castle/core/config.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/io/fd/descriptor.hpp"
#include "castle/utility/move.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <termios.h>
    #include <stdint.h>
#endif

namespace castle
{
namespace io
{
namespace uart
{

enum class data_bits : unsigned char
{
    five = 5U,
    six = 6U,
    seven = 7U,
    eight = 8U
};

enum class parity : unsigned char
{
    none = 0U,
    even,
    odd,
    mark,
    space
};

enum class stop_bits : unsigned char
{
    one = 1U,
    two = 2U
};

enum class flow_control : unsigned char
{
    none = 0U,
    software,
    hardware
};

/** @brief Fixed-size UART configuration; `device_path` is borrowed, not copied. */
struct uart_config
{
    CASTLE_CONST char* device_path;
    uint32_t baud_rate;
    data_bits data;
    parity parity_mode;
    stop_bits stop;
    flow_control flow;
    uint8_t min_read_bytes;
    uint8_t read_timeout_deciseconds;
    bool non_blocking;

    CASTLE_CONSTEXPR explicit uart_config(
        CASTLE_CONST char* path = nullptr,
        uint32_t baud = 115200U,
        data_bits bits = data_bits::eight,
        parity parity_value = parity::none,
        stop_bits stop_value = stop_bits::one,
        flow_control flow_value = flow_control::none,
        uint8_t min_bytes = 0U,
        uint8_t timeout_deciseconds = 0U,
        bool use_non_blocking = false) CASTLE_NOEXCEPT
        : device_path(path)
        , baud_rate(baud)
        , data(bits)
        , parity_mode(parity_value)
        , stop(stop_value)
        , flow(flow_value)
        , min_read_bytes(min_bytes)
        , read_timeout_deciseconds(timeout_deciseconds)
        , non_blocking(use_non_blocking)
    {
    }
};

namespace detail
{

#if CASTLE_USING_POSIX_APIS
CASTLE_INLINE bool baud_to_native(uint32_t baud, speed_t& native) CASTLE_NOEXCEPT
{
    switch (baud)
    {
#ifdef B50
        case 50U: native = B50; return true;
#endif
#ifdef B75
        case 75U: native = B75; return true;
#endif
#ifdef B110
        case 110U: native = B110; return true;
#endif
#ifdef B134
        case 134U: native = B134; return true;
#endif
#ifdef B150
        case 150U: native = B150; return true;
#endif
#ifdef B200
        case 200U: native = B200; return true;
#endif
#ifdef B300
        case 300U: native = B300; return true;
#endif
#ifdef B600
        case 600U: native = B600; return true;
#endif
#ifdef B1200
        case 1200U: native = B1200; return true;
#endif
#ifdef B1800
        case 1800U: native = B1800; return true;
#endif
#ifdef B2400
        case 2400U: native = B2400; return true;
#endif
#ifdef B4800
        case 4800U: native = B4800; return true;
#endif
#ifdef B9600
        case 9600U: native = B9600; return true;
#endif
#ifdef B19200
        case 19200U: native = B19200; return true;
#endif
#ifdef B38400
        case 38400U: native = B38400; return true;
#endif
#ifdef B57600
        case 57600U: native = B57600; return true;
#endif
#ifdef B115200
        case 115200U: native = B115200; return true;
#endif
#ifdef B230400
        case 230400U: native = B230400; return true;
#endif
#ifdef B460800
        case 460800U: native = B460800; return true;
#endif
#ifdef B500000
        case 500000U: native = B500000; return true;
#endif
#ifdef B576000
        case 576000U: native = B576000; return true;
#endif
#ifdef B921600
        case 921600U: native = B921600; return true;
#endif
#ifdef B1000000
        case 1000000U: native = B1000000; return true;
#endif
#ifdef B1152000
        case 1152000U: native = B1152000; return true;
#endif
#ifdef B1500000
        case 1500000U: native = B1500000; return true;
#endif
#ifdef B2000000
        case 2000000U: native = B2000000; return true;
#endif
#ifdef B2500000
        case 2500000U: native = B2500000; return true;
#endif
#ifdef B3000000
        case 3000000U: native = B3000000; return true;
#endif
#ifdef B3500000
        case 3500000U: native = B3500000; return true;
#endif
#ifdef B4000000
        case 4000000U: native = B4000000; return true;
#endif
        default: return false;
    }
}
#endif


CASTLE_INLINE bool valid_baud_value(uint32_t baud) CASTLE_NOEXCEPT
{
    switch (baud)
    {
        case 50U: CASTLE_FALL_THROUGH;
        case 75U: CASTLE_FALL_THROUGH;
        case 110U: CASTLE_FALL_THROUGH;
        case 134U: CASTLE_FALL_THROUGH;
        case 150U: CASTLE_FALL_THROUGH;
        case 200U: CASTLE_FALL_THROUGH;
        case 300U: CASTLE_FALL_THROUGH;
        case 600U: CASTLE_FALL_THROUGH;
        case 1200U: CASTLE_FALL_THROUGH;
        case 1800U: CASTLE_FALL_THROUGH;
        case 2400U: CASTLE_FALL_THROUGH;
        case 4800U: CASTLE_FALL_THROUGH;
        case 9600U: CASTLE_FALL_THROUGH;
        case 19200U: CASTLE_FALL_THROUGH;
        case 38400U: CASTLE_FALL_THROUGH;
        case 57600U: CASTLE_FALL_THROUGH;
        case 115200U: CASTLE_FALL_THROUGH;
        case 230400U: CASTLE_FALL_THROUGH;
        case 460800U: CASTLE_FALL_THROUGH;
        case 500000U: CASTLE_FALL_THROUGH;
        case 576000U: CASTLE_FALL_THROUGH;
        case 921600U: CASTLE_FALL_THROUGH;
        case 1000000U: CASTLE_FALL_THROUGH;
        case 1152000U: CASTLE_FALL_THROUGH;
        case 1500000U: CASTLE_FALL_THROUGH;
        case 2000000U: CASTLE_FALL_THROUGH;
        case 2500000U: CASTLE_FALL_THROUGH;
        case 3000000U: CASTLE_FALL_THROUGH;
        case 3500000U: CASTLE_FALL_THROUGH;
        case 4000000U: return true;
        default: return false;
    }
}

CASTLE_INLINE bool valid_data_bits(data_bits value) CASTLE_NOEXCEPT
{
    return value == data_bits::five || value == data_bits::six
        || value == data_bits::seven || value == data_bits::eight;
}

CASTLE_INLINE bool valid_parity(parity value) CASTLE_NOEXCEPT
{
    return value == parity::none || value == parity::even || value == parity::odd
        || value == parity::mark || value == parity::space;
}

CASTLE_INLINE bool valid_stop_bits(stop_bits value) CASTLE_NOEXCEPT
{
    return value == stop_bits::one || value == stop_bits::two;
}

CASTLE_INLINE bool valid_flow_control(flow_control value) CASTLE_NOEXCEPT
{
    return value == flow_control::none || value == flow_control::software
        || value == flow_control::hardware;
}

} // namespace detail

/** @brief Validates generic UART fields and, on POSIX, baud-rate availability. */
CASTLE_INLINE castle::status validate_config(uart_config const& config) CASTLE_NOEXCEPT
{
    if (config.device_path == nullptr || config.device_path[0] == '\0' || !detail::valid_baud_value(config.baud_rate)
     || !detail::valid_data_bits(config.data) || !detail::valid_parity(config.parity_mode)
     || !detail::valid_stop_bits(config.stop) || !detail::valid_flow_control(config.flow))
    {
        return castle::status::invalid_config;
    }
#if CASTLE_USING_POSIX_APIS
    speed_t native_speed = 0;
    if (!detail::baud_to_native(config.baud_rate, native_speed)) // LCOV_EXCL_BR_LINE
    {
        return castle::status::invalid_config;
    }
#ifndef CRTSCTS
    if (config.flow == flow_control::hardware)
    {
        return castle::status::invalid_config;
    }
#endif
#ifndef CMSPAR
    if (config.parity_mode == parity::mark || config.parity_mode == parity::space)
    {
        return castle::status::invalid_config;
    }
#endif
#endif
    return castle::status::ok;
}

/** @brief True when this build can apply POSIX termios UART settings. */
static CASTLE_CONSTEXPR bool uart_available = (CASTLE_USING_POSIX_APIS != 0);

class basic_uart
{
public:
    using size_type = castle::size_type;

    basic_uart() CASTLE_DEFAULT;
    basic_uart(basic_uart const&) CASTLE_DELETE;
    basic_uart& operator=(basic_uart const&) CASTLE_DELETE;
    basic_uart(basic_uart&& other) CASTLE_NOEXCEPT
        : descriptor_(castle::move(other.descriptor_))
        , config_(other.config_)
        , configured_(other.configured_)
    {
        other.configured_ = false;
    }

    basic_uart& operator=(basic_uart&& other) CASTLE_NOEXCEPT
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
        {
            descriptor_ = castle::move(other.descriptor_);
            config_ = other.config_;
            configured_ = other.configured_;
            other.configured_ = false;
        }
        return *this;
    }

    /** @brief Opens the UART and applies the requested termios configuration. */
    castle::status open(uart_config const& config, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::status result = validate_config(config);
        // LCOV_EXCL_START
        if (!castle::succeeded(result))
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return result;
        }
        if (descriptor_.is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
        // LCOV_EXCL_STOP
#if CASTLE_USING_POSIX_APIS
        int flags = O_RDWR;
#ifdef O_NOCTTY
        flags |= O_NOCTTY;
#endif
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
#ifdef O_NONBLOCK
        if (config.non_blocking)
        {
            flags |= O_NONBLOCK;
        }
#else
        if (config.non_blocking)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#endif
        fd::basic_fd next;
        result = next.open(config.device_path, flags, 0666U, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
#ifndef O_CLOEXEC
        result = next.set_close_on_exec(true, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
#endif
        result = apply_config(next.native_handle(), config, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
        descriptor_ = castle::move(next);
        config_ = config;
        configured_ = true;
        return castle::status::ok;
#else
        (void)config;
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Reapplies termios settings to an open UART. */
    castle::status configure(uart_config const& config, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::status result = validate_config(config);
        // LCOV_EXCL_START
        if (!castle::succeeded(result))
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return result;
        }
        if (!descriptor_.is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
        // LCOV_EXCL_STOP
#if CASTLE_USING_POSIX_APIS
        result = apply_config(descriptor_.native_handle(), config, system_error);
        if (castle::succeeded(result))
        {
            result = descriptor_.set_nonblocking(config.non_blocking, system_error);
        }
        if (castle::succeeded(result))
        {
            config_ = config;
            configured_ = true;
        }
        return result;
#else
        (void)config;
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Closes the UART descriptor. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        configured_ = false;
        return descriptor_.close(system_error);
    }

    /** @brief Returns whether the UART descriptor is open. */
    CASTLE_NODISCARD bool is_open() const CASTLE_NOEXCEPT { return descriptor_.is_open(); }
    /** @brief Returns whether termios configuration has been applied successfully. */
    CASTLE_NODISCARD bool is_configured() const CASTLE_NOEXCEPT { return configured_ && descriptor_.is_open(); }
    /** @brief Returns the native UART descriptor, or -1 when closed. */
    CASTLE_NODISCARD int native_handle() const CASTLE_NOEXCEPT { return descriptor_.native_handle(); }
    /** @brief Returns the last successfully applied configuration. */
    CASTLE_NODISCARD uart_config const& configuration() const CASTLE_NOEXCEPT { return config_; }

    /** @brief Reads up to `capacity` bytes from the UART. */
    castle::status read_some(void* buffer, size_type capacity, size_type& bytes_read,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return descriptor_.read_some(buffer, capacity, bytes_read, system_error);
    }

    /** @brief Writes up to `size` bytes to the UART. */
    castle::status write_some(CASTLE_CONST void* buffer, size_type size, size_type& bytes_written,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return descriptor_.write_some(buffer, size, bytes_written, system_error);
    }

    /** @brief Writes all bytes unless a system error or non-blocking back-pressure occurs. */
    castle::status write_all(CASTLE_CONST void* buffer, size_type size, size_type& bytes_written,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return descriptor_.write_all(buffer, size, bytes_written, system_error);
    }

    /** @brief Discards pending received bytes. */
    castle::status flush_input(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
#if CASTLE_USING_POSIX_APIS
        return flush(TCIFLUSH, system_error);
#else
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Discards pending transmit bytes. */
    castle::status flush_output(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
#if CASTLE_USING_POSIX_APIS
        return flush(TCOFLUSH, system_error);
#else
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Discards both receive and transmit queues. */
    castle::status flush(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
#if CASTLE_USING_POSIX_APIS
        return flush(TCIOFLUSH, system_error);
#else
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Waits until queued output has been transmitted. */
    castle::status drain(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#if CASTLE_USING_POSIX_APIS
        if (::tcdrain(native_handle()) != 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::ok;
#else
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
        // LCOV_EXCL_STOP
    }

    /** @brief Returns the number of bytes currently waiting in the receive queue. */
    castle::status bytes_available(size_type& count, int* system_error = nullptr) const CASTLE_NOEXCEPT
    {
        count = 0U;
        // LCOV_EXCL_START
        if (!is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#if CASTLE_USING_POSIX_APIS && defined(FIONREAD)
        int pending = 0;
        if (::ioctl(native_handle(), FIONREAD, &pending) != 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (pending < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::data_loss;
        }
        count = static_cast<size_type>(pending);
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::ok;
#else
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
        // LCOV_EXCL_STOP
    }

private:
#if CASTLE_USING_POSIX_APIS
    static castle::status apply_config(int descriptor, uart_config const& config,
                                       int* system_error) CASTLE_NOEXCEPT
    {
        termios options;
        if (::tcgetattr(descriptor, &options) != 0)
        {
            // LCOV_EXCL_START
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            // LCOV_EXCL_STOP
            return castle::status::system_call_error;
        }

        speed_t speed = 0;
        // LCOV_EXCL_START
        if (!detail::baud_to_native(config.baud_rate, speed))
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
        // LCOV_EXCL_STOP

        options.c_iflag = 0;
        options.c_oflag = 0;
        options.c_lflag = 0;
        options.c_cflag &= static_cast<tcflag_t>(~(CSIZE | PARENB | PARODD | CSTOPB));
#ifdef CMSPAR
        options.c_cflag &= static_cast<tcflag_t>(~CMSPAR);
#endif
#ifdef CRTSCTS
        options.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
#endif
        options.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);

        switch (config.data)
        {
            case data_bits::five: options.c_cflag |= CS5; break;
            case data_bits::six: options.c_cflag |= CS6; break;
            case data_bits::seven: options.c_cflag |= CS7; break;
            case data_bits::eight: options.c_cflag |= CS8; break;
            // LCOV_EXCL_START
            default:
                if (system_error != nullptr)
                {
                    *system_error = 0;
                }
                return castle::status::invalid_config;
            // LCOV_EXCL_STOP
        }

        switch (config.parity_mode)
        {
            case parity::none: break;
            case parity::even:
                options.c_cflag |= PARENB;
                options.c_iflag |= INPCK;
                break;
            case parity::odd:
                options.c_cflag |= static_cast<tcflag_t>(PARENB | PARODD);
                options.c_iflag |= INPCK;
                break;
            case parity::mark:
#ifdef CMSPAR
                options.c_cflag |= static_cast<tcflag_t>(PARENB | PARODD | CMSPAR);
                options.c_iflag |= INPCK;
#else
                if (system_error != nullptr)
                {
                    *system_error = 0;
                }
                return castle::status::invalid_config;
#endif
                break;
            case parity::space:
#ifdef CMSPAR
                options.c_cflag |= static_cast<tcflag_t>(PARENB | CMSPAR);
                options.c_iflag |= INPCK;
#else
                if (system_error != nullptr)
                {
                    *system_error = 0;
                }
                return castle::status::invalid_config;
#endif
                break;
            // LCOV_EXCL_START
            default:
                if (system_error != nullptr)
                {
                    *system_error = 0;
                }
                return castle::status::invalid_config;
            // LCOV_EXCL_STOP
        }

        if (config.stop == stop_bits::two)
        {
            options.c_cflag |= CSTOPB;
        }
        else if (config.stop != stop_bits::one)
        {
            // LCOV_EXCL_START
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
            // LCOV_EXCL_STOP
        }

        switch (config.flow)
        {
            case flow_control::none: break;
            case flow_control::software:
                options.c_iflag |= static_cast<tcflag_t>(IXON | IXOFF);
                break;
            case flow_control::hardware:
#ifdef CRTSCTS
                options.c_cflag |= CRTSCTS;
#else
                if (system_error != nullptr)
                {
                    *system_error = 0;
                }
                return castle::status::invalid_config;
#endif
                break;
            // LCOV_EXCL_START
            default:
                if (system_error != nullptr)
                {
                    *system_error = 0;
                }
                return castle::status::invalid_config;
            // LCOV_EXCL_STOP
        }

        options.c_cc[VMIN] = config.min_read_bytes;
        options.c_cc[VTIME] = config.read_timeout_deciseconds;
        // LCOV_EXCL_START
        if (::cfsetispeed(&options, speed) != 0 || ::cfsetospeed(&options, speed) != 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (::tcsetattr(descriptor, TCSANOW, &options) != 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::ok;
        // LCOV_EXCL_STOP
    }
#endif

    castle::status flush(int queue, int* system_error) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#if CASTLE_USING_POSIX_APIS
        if (::tcflush(native_handle(), queue) != 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::ok;
#else
        (void)queue;
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
        // LCOV_EXCL_STOP
    }

    fd::basic_fd descriptor_;
    uart_config config_;
    bool configured_ = false;
};

using uart = basic_uart;

} // namespace uart
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_UART_UART_HPP
