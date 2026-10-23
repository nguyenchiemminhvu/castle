// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file i2c.hpp
 * @brief Allocation-free Linux i2c-dev configuration and combined transfers.
 *
 * `basic_i2c` owns an i2c-dev descriptor bound to one 7-bit or 10-bit target
 * address. It supports direct byte-stream reads/writes and a combined
 * write-then-read transaction using `I2C_RDWR` (for register-addressed devices).
 * Linux UAPI headers are included only when available; unsupported builds keep
 * the API parseable and return `status::not_configured`.
 */
#ifndef CASTLE_EXT_IO_I2C_I2C_HPP
#define CASTLE_EXT_IO_I2C_I2C_HPP

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
    #include <stdint.h>
#endif

#if CASTLE_USING_IO_I2C_VIA_I2CDEV
    #include <linux/i2c.h>
    #include <linux/i2c-dev.h>
#endif

namespace castle
{
namespace io
{
namespace i2c
{

/** @brief Fixed-size I2C adapter and target-address configuration. */
struct i2c_config
{
    CASTLE_CONST char* device_path;
    uint16_t address;
    bool ten_bit_address;

    CASTLE_CONSTEXPR explicit i2c_config(CASTLE_CONST char* path = nullptr,
                                          uint16_t slave_address = 0x42U,
                                          bool use_ten_bit_address = false) CASTLE_NOEXCEPT
        : device_path(path)
        , address(slave_address)
        , ten_bit_address(use_ten_bit_address)
    {
    }
};

/** @brief Validates the path and I2C address range without opening hardware. */
CASTLE_INLINE castle::status validate_config(i2c_config CASTLE_CONST& config) CASTLE_NOEXCEPT
{
    if (config.device_path == nullptr || config.device_path[0] == '\0')
    {
        return castle::status::invalid_config;
    }
    if (config.ten_bit_address)
    {
        return (config.address <= 0x03FFU) ? castle::status::ok : castle::status::invalid_config;
    }
    return (config.address <= 0x007FU) ? castle::status::ok : castle::status::invalid_config;
}

/** @brief True when the Linux i2c-dev UAPI is available in this build. */
static CASTLE_CONSTEXPR bool i2c_available = (CASTLE_USING_IO_I2C_VIA_I2CDEV != 0);

class basic_i2c
{
public:
    using size_type = castle::size_type;

    basic_i2c() CASTLE_DEFAULT;
    basic_i2c(basic_i2c CASTLE_CONST&) CASTLE_DELETE;
    basic_i2c& operator=(basic_i2c CASTLE_CONST&) CASTLE_DELETE;
    basic_i2c(basic_i2c&& other) CASTLE_NOEXCEPT
        : descriptor_(CASTLE_MOVE(other.descriptor_))
        , config_(other.config_)
        , configured_(other.configured_)
    {
        other.configured_ = false;
    }

    basic_i2c& operator=(basic_i2c&& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            descriptor_ = CASTLE_MOVE(other.descriptor_);
            config_ = other.config_;
            configured_ = other.configured_;
            other.configured_ = false;
        }
        return *this;
    }

    /** @brief Opens the adapter, selects the target address and applies address width. */
    castle::status open(i2c_config CASTLE_CONST& config, int* system_error = nullptr) CASTLE_NOEXCEPT
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
#if CASTLE_USING_IO_I2C_VIA_I2CDEV
        fd::basic_fd next;
        result = next.open(config.device_path, O_RDWR
#ifdef O_CLOEXEC
                           | O_CLOEXEC
#endif
                           , 0666U, system_error);
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
        // LCOV_EXCL_START
        if (!castle::succeeded(result))
        {
            return result;
        }
        descriptor_ = CASTLE_MOVE(next);
        config_ = config;
        configured_ = true;
        return castle::status::ok;
        // LCOV_EXCL_STOP
#else
        (void)config;
        if (system_error != nullptr) { *system_error = 0; }
        return castle::status::not_configured;
#endif
    }

    /** @brief Re-selects the target address of an open adapter. */
    castle::status configure(i2c_config CASTLE_CONST& config, int* system_error = nullptr) CASTLE_NOEXCEPT
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
        if (!is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_I2C_VIA_I2CDEV
        result = apply_config(native_handle(), config, system_error);
        if (castle::succeeded(result))
        {
            config_ = config;
            configured_ = true;
        }
        return result;
        // LCOV_EXCL_STOP
#else
        (void)config;
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Closes the adapter descriptor. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        configured_ = false;
        return descriptor_.close(system_error);
    }

    /** @brief Returns whether the descriptor is open. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.is_open(); }
    /** @brief Returns whether an I2C target is currently selected. */
    CASTLE_NODISCARD bool is_configured() CASTLE_CONST CASTLE_NOEXCEPT { return configured_ && descriptor_.is_open(); } // LCOV_EXCL_BR_LINE
    /** @brief Returns the native descriptor, or -1 when closed. */
    CASTLE_NODISCARD int native_handle() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.native_handle(); } // LCOV_EXCL_LINE
    /** @brief Returns the selected adapter/address configuration. */
    CASTLE_NODISCARD i2c_config CASTLE_CONST& configuration() CASTLE_CONST CASTLE_NOEXCEPT { return config_; }

    /** @brief Reads up to `capacity` bytes from the selected target. */
    castle::status read_some(void* buffer, size_type capacity, size_type& bytes_read,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_configured())
        {
            bytes_read = 0U;
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
        return descriptor_.read_some(buffer, capacity, bytes_read, system_error);
        // LCOV_EXCL_STOP
    }

    /** @brief Writes up to `size` bytes to the selected target. */
    castle::status write_some(CASTLE_CONST void* buffer, size_type size, size_type& bytes_written,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_configured())
        {
            bytes_written = 0U;
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
        return descriptor_.write_some(buffer, size, bytes_written, system_error);
        // LCOV_EXCL_STOP
    }

    /**
     * @brief Performs one combined I2C write followed by repeated-start read.
     * @param transmit_buffer Bytes sent before the read phase (often a register address).
     * @param transmit_size Number of transmit bytes; must be non-zero.
     * @param receive_buffer Destination for the read phase.
     * @param receive_size Number of bytes to receive; must be non-zero.
     * @return `ok` on the complete two-message transaction, `data_loss` if the
     *         adapter reports an incomplete message count, or an error status.
     */
    castle::status write_read(CASTLE_CONST void* transmit_buffer,
                              size_type transmit_size,
                              void* receive_buffer,
                              size_type receive_size,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (transmit_buffer == nullptr || receive_buffer == nullptr
            || transmit_size == 0U || receive_size == 0U
            || transmit_size > 0xFFFFU || receive_size > 0xFFFFU)
        {
            // LCOV_EXCL_START
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_argument;
        }
        if (!is_configured())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_I2C_VIA_I2CDEV
        struct i2c_msg messages[2];
        messages[0].addr = config_.address;
        messages[0].flags = config_.ten_bit_address ? I2C_M_TEN : 0U;
        messages[0].len = static_cast<uint16_t>(transmit_size);
        messages[0].buf = static_cast<uint8_t*>(const_cast<void*>(transmit_buffer));
        messages[1].addr = config_.address;
        messages[1].flags = static_cast<uint16_t>(I2C_M_RD | (config_.ten_bit_address ? I2C_M_TEN : 0U));
        messages[1].len = static_cast<uint16_t>(receive_size);
        messages[1].buf = static_cast<uint8_t*>(receive_buffer);
        struct i2c_rdwr_ioctl_data transaction;
        transaction.msgs = messages;
        transaction.nmsgs = 2U;
        CASTLE_CONST int result = ::ioctl(native_handle(), I2C_RDWR, &transaction);
        if (result < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (result != 2)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::data_loss;
        }
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::ok;
        // LCOV_EXCL_STOP
#else
        (void)transmit_buffer; (void)transmit_size; (void)receive_buffer; (void)receive_size;
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Reads adapter functionality flags (`I2C_FUNCS`). */
    castle::status get_functionality(uint32_t& flags, int* system_error = nullptr) CASTLE_CONST CASTLE_NOEXCEPT
    {
        flags = 0U;
        // LCOV_EXCL_START
        if (!is_configured())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_I2C_VIA_I2CDEV
        unsigned long native_flags = 0UL;
        if (::ioctl(native_handle(), I2C_FUNCS, &native_flags) != 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        flags = static_cast<uint32_t>(native_flags);
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::ok;
        // LCOV_EXCL_STOP
#else
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

private:
#if CASTLE_USING_IO_I2C_VIA_I2CDEV
    static castle::status apply_config(int descriptor, i2c_config CASTLE_CONST& config,
                                       int* system_error) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        CASTLE_CONST unsigned long ten_bit = config.ten_bit_address ? 1UL : 0UL;
        if (::ioctl(descriptor, I2C_TENBIT, ten_bit) < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (::ioctl(descriptor, I2C_SLAVE, static_cast<unsigned long>(config.address)) < 0)
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

    fd::basic_fd descriptor_;
    i2c_config config_;
    bool configured_ = false;
};

using i2c = basic_i2c;

} // namespace i2c
} // namespace io
} // namespace castle

#undef CASTLE_USING_IO_I2C_VIA_I2CDEV

#endif // CASTLE_EXT_IO_I2C_I2C_HPP
