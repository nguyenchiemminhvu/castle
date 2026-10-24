// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file types.hpp
 * @brief Platform-neutral CAN configuration, bus-state and timeout types.
 *
 * These value types are shared by `basic_bus` and every backend, so application code keeps one
 * vocabulary whether it runs on Linux SocketCAN, Zephyr or the host-side mock.
 */
#ifndef CASTLE_EXT_IO_CAN_TYPES_HPP
#define CASTLE_EXT_IO_CAN_TYPES_HPP

#include "castle/chrono/duration.hpp"
#include "castle/core/compiler.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace io
{
namespace can
{

/** @brief Timeout type used by `send()` and `receive()`. */
using timeout_type = castle::chrono::milliseconds;

/** @brief Timeout that never blocks: perform the operation once and report `empty`/`full` if it cannot proceed. */
static CASTLE_INLINE CASTLE_CONSTEXPR timeout_type no_wait = timeout_type(0);

/** @brief Timeout that blocks until the operation can proceed. */
static CASTLE_INLINE CASTLE_CONSTEXPR timeout_type wait_forever = timeout_type::max();

/** @name Common classic-CAN bit rates in bit/s. */
///@{
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t bitrate_10k   = 10000U;
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t bitrate_20k   = 20000U;
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t bitrate_50k   = 50000U;
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t bitrate_100k  = 100000U;
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t bitrate_125k  = 125000U;
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t bitrate_250k  = 250000U;
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t bitrate_500k  = 500000U;
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t bitrate_800k  = 800000U;
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t bitrate_1m    = 1000000U;
///@}

/** @brief Highest bit rate supported by classic CAN. */
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t max_bitrate = bitrate_1m;

/** @brief Controller operating mode. */
enum class can_mode : uint8_t
{
    normal = 0U, /**< Transmit and receive on the bus. */
    loopback,    /**< Transmitted frames are looped back to this node and not driven onto the bus. */
    listen_only  /**< Receive only; the controller never drives the bus (sniffing). */
};

/**
 * @brief Fixed-size open-time configuration.
 *
 * `bitrate` is applied by backends that own bit timing (Zephyr `can_set_bitrate()`); zero keeps the
 * driver/device-tree default. On Linux the interface owns bit timing
 * (`ip link set can0 type can bitrate 500000`), so the value is not applied there.
 */
struct can_config
{
    uint32_t bitrate;
    can_mode mode;

    CASTLE_CONSTEXPR explicit can_config(uint32_t bit_rate = 0U,
                                         can_mode operating_mode = can_mode::normal) CASTLE_NOEXCEPT
        : bitrate(bit_rate)
        , mode(operating_mode)
    {
    }
};

/** @brief Validates a configuration without touching hardware: known mode and a bit rate of at most 1 Mbit/s. */
CASTLE_NODISCARD CASTLE_INLINE castle::status validate_config(can_config CASTLE_CONST& config) CASTLE_NOEXCEPT
{
    if (config.bitrate > max_bitrate
        || (config.mode != can_mode::normal
            && config.mode != can_mode::loopback
            && config.mode != can_mode::listen_only))
    {
        return castle::status::invalid_config;
    }
    return castle::status::ok;
}

/** @brief CAN controller error state (ISO 11898-1 fault confinement). */
enum class bus_state : uint8_t
{
    stopped = 0U,  /**< Not open, or the controller is stopped. */
    error_active,  /**< Normal operation. */
    error_warning, /**< An error counter reached the warning level (96). */
    error_passive, /**< An error counter reached 128; the node signals errors passively. */
    bus_off        /**< Transmit error counter exceeded 255; the node is disconnected from the bus. */
};

/** @brief Snapshot of controller health returned by `query_status()`. */
struct bus_status
{
    bus_state state;
    uint8_t tx_error_counter;
    uint8_t rx_error_counter;
    uint32_t rx_overruns;
    uint32_t tx_failures;

    CASTLE_CONSTEXPR bus_status() CASTLE_NOEXCEPT
        : state(bus_state::stopped)
        , tx_error_counter(0U)
        , rx_error_counter(0U)
        , rx_overruns(0U)
        , tx_failures(0U)
    {
    }
};

/** @brief Returns true when the controller is in `error_active` or `error_warning` and can still transmit normally. */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_operational(bus_state value) CASTLE_NOEXCEPT
{
    return value == bus_state::error_active || value == bus_state::error_warning;
}

/** @brief Returns true when `timeout` is the `wait_forever` sentinel. */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_forever(timeout_type timeout) CASTLE_NOEXCEPT
{
    return timeout.count() == wait_forever.count();
}

} // namespace can
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_CAN_TYPES_HPP
