// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file can.hpp
 * @brief Umbrella header for Castle's CAN extension with compile-time backend selection.
 *
 * Includes the platform-neutral frame, filter, configuration and bus facade types plus the mock
 * backend, and selects `default_backend` from the build configuration:
 *
 * | Condition                                 | `default_backend`      |
 * |-------------------------------------------|------------------------|
 * | `CASTLE_USING_IO_CAN_VIA_ZEPHYR`          | `zephyr::backend`      |
 * | `CASTLE_USING_IO_CAN_VIA_SOCKET_CAN`      | `socket_can::backend`  |
 * | neither                                   | `mock::backend`        |
 *
 * `CASTLE_USING_IO_CAN_VIA_ZEPHYR` is detected from `__ZEPHYR__` and the presence of
 * `<zephyr/drivers/can.h>`; `CASTLE_USING_IO_CAN_VIA_SOCKET_CAN` from Linux POSIX builds with
 * `<linux/can.h>` (both defined in `castle/core/config.hpp` and overridable on the command line).
 * Application code written against `default_bus` therefore compiles unchanged on each platform;
 * only the endpoint passed to `open()` differs (`endpoint_type`).
 *
 * @code
 * #include "castle_ext/comm/can/can.hpp"
 *
 * castle::comm::can::default_bus bus;
 * bus.open(endpoint, castle::comm::can::can_config(castle::comm::can::bitrate_500k));
 * bus.accept_all();
 * @endcode
 */
#ifndef CASTLE_EXT_COMM_CAN_CAN_HPP
#define CASTLE_EXT_COMM_CAN_CAN_HPP

#include "castle/core/config.hpp"
#include "castle_ext/comm/can/backends/mock.hpp"
#include "castle_ext/comm/can/bus.hpp"
#include "castle_ext/comm/can/filter.hpp"
#include "castle_ext/comm/can/frame.hpp"
#include "castle_ext/comm/can/types.hpp"

#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
    #include "castle_ext/comm/can/backends/zephyr.hpp"
#elif CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
    #include "castle_ext/comm/can/backends/socket_can.hpp"
#endif

namespace castle
{
namespace comm
{
namespace can
{

#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
using default_backend = zephyr::backend;
#elif CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
using default_backend = socket_can::backend;
#else
using default_backend = mock::backend;
#endif

/** @brief True when `default_backend` drives real CAN hardware rather than the host-side mock. */
static CASTLE_INLINE CASTLE_CONSTEXPR bool default_backend_is_hardware =
    (CASTLE_USING_IO_CAN_VIA_ZEPHYR != 0) || (CASTLE_USING_IO_CAN_VIA_SOCKET_CAN != 0);

/** @brief Bus facade over the backend selected for this build. */
using default_bus = basic_bus<default_backend>;

} // namespace can
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_CAN_CAN_HPP
