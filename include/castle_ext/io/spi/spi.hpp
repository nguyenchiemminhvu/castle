// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file spi.hpp
 * @brief Allocation-free Linux spidev configuration and full-duplex transfer.
 *
 * `basic_spi` owns a spidev descriptor, configures its mode/word size/speed and
 * exposes bounded synchronous transfers through `SPI_IOC_MESSAGE(1)`. The
 * platform-specific UAPI is included only on Linux POSIX builds where the
 * spidev header is available. Other targets retain the API as a safe stub.
 */
#ifndef CASTLE_EXT_IO_SPI_SPI_HPP
#define CASTLE_EXT_IO_SPI_SPI_HPP

#include "castle/core/config.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/io/fd/descriptor.hpp"
#include "castle/utility/move.hpp"

#include <limits.h>

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <stdint.h>
#endif

#if CASTLE_USING_IO_SPI_VIA_SPIDEV
    #include <linux/spi/spidev.h>
#endif

namespace castle
{
namespace io
{
namespace spi
{

enum class mode : unsigned char
{
    mode0 = 0U,
    mode1 = 1U,
    mode2 = 2U,
    mode3 = 3U
};

/** @brief Fixed-size configuration for one Linux spidev endpoint. */
struct spi_config
{
    CASTLE_CONST char* device_path;
    mode transfer_mode;
    uint8_t bits_per_word;
    uint32_t max_speed_hz;
    bool lsb_first;
    bool chip_select_high;
    bool three_wire;
    bool loopback;
    bool no_chip_select;
    bool ready_signal;

    CASTLE_CONSTEXPR explicit spi_config(
        CASTLE_CONST char* path = nullptr,
        mode spi_mode = mode::mode0,
        uint8_t word_bits = 8U,
        uint32_t speed = 1000000U,
        bool lsb = false,
        bool cs_high = false,
        bool three = false,
        bool loop = false,
        bool no_cs = false,
        bool ready = false) CASTLE_NOEXCEPT
        : device_path(path)
        , transfer_mode(spi_mode)
        , bits_per_word(word_bits)
        , max_speed_hz(speed)
        , lsb_first(lsb)
        , chip_select_high(cs_high)
        , three_wire(three)
        , loopback(loop)
        , no_chip_select(no_cs)
        , ready_signal(ready)
    {
    }
};

/** @brief Per-transfer overrides; zero selects the values from `spi_config`. */
struct transfer_config
{
    uint16_t delay_microseconds;
    uint8_t bits_per_word;
    uint32_t speed_hz;
    bool chip_select_change;

    CASTLE_CONSTEXPR explicit transfer_config(uint16_t delay = 0U,
                                               uint8_t word_bits = 0U,
                                               uint32_t speed = 0U,
                                               bool cs_change = false) CASTLE_NOEXCEPT
        : delay_microseconds(delay)
        , bits_per_word(word_bits)
        , speed_hz(speed)
        , chip_select_change(cs_change)
    {
    }
};

/** @brief Validates portable SPI configuration fields without opening hardware. */
CASTLE_INLINE castle::status validate_config(spi_config CASTLE_CONST& config) CASTLE_NOEXCEPT
{
    if (config.device_path == nullptr
     || config.device_path[0] == '\0' // LCOV_EXCL_BR_LINE
     || static_cast<unsigned char>(config.transfer_mode) > 3U
     || config.bits_per_word == 0U
     || config.bits_per_word > 32U
     || config.max_speed_hz == 0U)
    {
        return castle::status::invalid_config;
    }
    return castle::status::ok;
}

/** @brief True when Linux spidev UAPI support is available in this build. */
static CASTLE_CONSTEXPR bool spi_available = (CASTLE_USING_IO_SPI_VIA_SPIDEV != 0);

class basic_spi
{
public:
    using size_type = castle::size_type;

    basic_spi() CASTLE_DEFAULT;
    basic_spi(basic_spi CASTLE_CONST&) CASTLE_DELETE;
    basic_spi& operator=(basic_spi CASTLE_CONST&) CASTLE_DELETE;
    basic_spi(basic_spi&& other) CASTLE_NOEXCEPT
        : descriptor_(CASTLE_MOVE(other.descriptor_))
        , config_(other.config_)
        , configured_(other.configured_)
    {
        other.configured_ = false;
    }

    basic_spi& operator=(basic_spi&& other) CASTLE_NOEXCEPT
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

    /** @brief Opens the spidev node and applies the configuration. */
    castle::status open(spi_config CASTLE_CONST& config, int* system_error = nullptr) CASTLE_NOEXCEPT
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
#if CASTLE_USING_IO_SPI_VIA_SPIDEV
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
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Reapplies SPI mode, word size and speed on an open device. */
    castle::status configure(spi_config CASTLE_CONST& config, int* system_error = nullptr) CASTLE_NOEXCEPT
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
#if CASTLE_USING_IO_SPI_VIA_SPIDEV
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

    /** @brief Closes the SPI descriptor. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        configured_ = false;
        return descriptor_.close(system_error);
    }

    /** @brief Returns whether the spidev descriptor is open. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.is_open(); }
    /** @brief Returns whether configuration has been successfully applied. */
    CASTLE_NODISCARD bool is_configured() CASTLE_CONST CASTLE_NOEXCEPT { return configured_ && descriptor_.is_open(); } // LCOV_EXCL_BR_LINE
    /** @brief Returns the native descriptor, or -1 when closed. */
    CASTLE_NODISCARD int native_handle() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.native_handle(); } // LCOV_EXCL_LINE
    /** @brief Returns the active SPI configuration. */
    CASTLE_NODISCARD spi_config CASTLE_CONST& configuration() CASTLE_CONST CASTLE_NOEXCEPT { return config_; }

    /** @brief Performs a single full-duplex spidev transfer. Either side may be null. */
    castle::status transfer(CASTLE_CONST void* transmit_buffer,
                            void* receive_buffer,
                            size_type size,
                            transfer_config CASTLE_CONST& options = transfer_config(),
                            int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (size == 0U)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::ok;
        }
        if ((transmit_buffer == nullptr && receive_buffer == nullptr) || size > static_cast<size_type>(INT_MAX)
            || (options.bits_per_word != 0U && options.bits_per_word > 32U))
        {
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
#if CASTLE_USING_IO_SPI_VIA_SPIDEV
        spi_ioc_transfer message = {};
        message.tx_buf = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(transmit_buffer));
        message.rx_buf = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(receive_buffer));
        message.len = static_cast<uint32_t>(size);
        message.delay_usecs = options.delay_microseconds;
        message.bits_per_word = options.bits_per_word == 0U ? config_.bits_per_word : options.bits_per_word;
        message.speed_hz = options.speed_hz == 0U ? config_.max_speed_hz : options.speed_hz;
        message.cs_change = options.chip_select_change ? 1U : 0U;
        CASTLE_CONST int result = ::ioctl(native_handle(), SPI_IOC_MESSAGE(1), &message);
        if (result < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (static_cast<size_type>(result) != size)
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
        (void)transmit_buffer; (void)receive_buffer; (void)options;
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Performs a raw single-direction POSIX read on the SPI descriptor. */
    castle::status read_some(void* buffer, size_type size, size_type& bytes_read,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return descriptor_.read_some(buffer, size, bytes_read, system_error);
    }

    /** @brief Performs a raw single-direction POSIX write on the SPI descriptor. */
    castle::status write_some(CASTLE_CONST void* buffer, size_type size, size_type& bytes_written,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return descriptor_.write_some(buffer, size, bytes_written, system_error);
    }

private:
#if CASTLE_USING_IO_SPI_VIA_SPIDEV
    static castle::status apply_config(int descriptor, spi_config CASTLE_CONST& config,
                                       int* system_error) CASTLE_NOEXCEPT
    {
        uint32_t native_mode = static_cast<uint32_t>(config.transfer_mode);
#ifdef SPI_CS_HIGH
        if (config.chip_select_high) // LCOV_EXCL_BR_LINE
        {
            native_mode |= SPI_CS_HIGH;
        }
#else
        if (config.chip_select_high)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
#endif
#ifdef SPI_3WIRE
        if (config.three_wire) // LCOV_EXCL_BR_LINE
        {
            native_mode |= SPI_3WIRE;
        }
#else
        if (config.three_wire)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
#endif
#ifdef SPI_LOOP
        if (config.loopback) // LCOV_EXCL_BR_LINE
        {
            native_mode |= SPI_LOOP;
        }
#else
        if (config.loopback)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
#endif
#ifdef SPI_NO_CS
        if (config.no_chip_select) // LCOV_EXCL_BR_LINE
        {
            native_mode |= SPI_NO_CS;
        }
#else
        if (config.no_chip_select)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
#endif
#ifdef SPI_READY
        if (config.ready_signal) // LCOV_EXCL_BR_LINE
        {
            native_mode |= SPI_READY;
        }
#else
        if (config.ready_signal)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
#endif

#ifdef SPI_IOC_WR_MODE32
        if (::ioctl(descriptor, SPI_IOC_WR_MODE32, &native_mode) < 0) // LCOV_EXCL_BR_LINE
#else
        uint8_t legacy_mode = static_cast<uint8_t>(native_mode);
        if (::ioctl(descriptor, SPI_IOC_WR_MODE, &legacy_mode) < 0)
#endif
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        // LCOV_EXCL_START
        uint8_t lsb = config.lsb_first ? 1U : 0U;
        if (::ioctl(descriptor, SPI_IOC_WR_LSB_FIRST, &lsb) < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        uint8_t word_bits = config.bits_per_word;
        if (::ioctl(descriptor, SPI_IOC_WR_BITS_PER_WORD, &word_bits) < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        uint32_t speed = config.max_speed_hz;
        if (::ioctl(descriptor, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
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
    spi_config config_;
    bool configured_ = false;
};

using spi = basic_spi;

} // namespace spi
} // namespace io
} // namespace castle

#undef CASTLE_USING_IO_SPI_VIA_SPIDEV

#endif // CASTLE_EXT_IO_SPI_SPI_HPP
