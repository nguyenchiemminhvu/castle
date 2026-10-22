// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file zephyr.hpp
 * @brief Zephyr RTOS CAN backend over `const struct device*` and the `can_*()` driver API.
 *
 * Requires Zephyr 3.2 or newer (`<zephyr/drivers/can.h>` with `can_start()`/`can_stop()` and the
 * `struct can_frame` API). Pass the controller from the devicetree, for example a MCP2515 on SPI:
 *
 * @code
 * static const struct device* const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
 *
 * castle::comm::can::bus<castle::comm::can::zephyr::backend> bus;
 * bus.open(can_dev, castle::comm::can::can_config(castle::comm::can::bitrate_500k));
 * bus.accept_all();
 * @endcode
 *
 * Mapping of the portable contract:
 * - `open()` stops the controller, applies mode and bit rate (zero keeps the devicetree
 *   `bus-speed`) and starts it. `close()` removes the filters and stops it.
 * - Every filter is registered with `can_add_rx_filter()` using one static callback. The
 *   callback converts the driver frame and pushes it into a fixed SPSC queue
 *   (`container::mirrored_ring_buffer`) and gives a `k_sem`; `receive()` takes the semaphore
 *   (honouring the timeout) and pops the queue. Nothing allocates and no thread is created.
 *   Frames that find the queue full are dropped and counted in `bus_status::rx_overruns`.
 * - `send()` queues the frame with a completion callback, so it returns as soon as a TX mailbox
 *   accepted it instead of waiting for a bus acknowledgement. Failed completions are counted in
 *   `bus_status::tx_failures`.
 * - Driver error codes map as: `-EAGAIN`/`-ENOSPC` to `full`, `-EINVAL` to `invalid_argument`,
 *   `-ENOTSUP`/`-ERANGE` to `invalid_config`, `-ENETDOWN` to `not_configured`, anything else to
 *   `system_call_error`; the positive errno is stored in `system_error`.
 *
 * Threading: the receive callback is the single producer (the MCP2515 driver runs it from its
 * interrupt thread; other drivers may use interrupt context, hence no locks are taken). Use one
 * consumer thread for `receive()`. `send()` is as thread safe as the driver's `can_send()`.
 * Filters that overlap deliver a frame once per matching filter.
 *
 * Without Zephyr (`CASTLE_USING_IO_CAN_VIA_ZEPHYR == 0`) the class is a stub whose operations
 * return `status::not_configured`.
 */
#ifndef CASTLE_EXT_COMM_CAN_BACKENDS_ZEPHYR_HPP
#define CASTLE_EXT_COMM_CAN_BACKENDS_ZEPHYR_HPP

#include "castle/container/mirrored_ring_buffer.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/sync/atomic.hpp"
#include "castle_ext/comm/can/detail/slot_table.hpp"
#include "castle_ext/comm/can/filter.hpp"
#include "castle_ext/comm/can/frame.hpp"
#include "castle_ext/comm/can/types.hpp"
#include "castle_ext/io/fd/control.hpp"

#include <errno.h>
#include <stdint.h>

#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
    #include <zephyr/device.h>
    #include <zephyr/drivers/can.h>
    #include <zephyr/kernel.h>
#else
    // Zephyr's device handle. Declared here so the API stays parseable on hosts.
    struct device;
#endif

namespace castle
{
namespace comm
{
namespace can
{
namespace zephyr
{

/** @brief True when the Zephyr CAN driver API is available in this build. */
static CASTLE_INLINE CASTLE_CONSTEXPR bool zephyr_available = (CASTLE_USING_IO_CAN_VIA_ZEPHYR != 0);

/** @brief Translation between Castle CAN types and Zephyr's driver API. */
namespace native
{

/**
 * @brief Maps a Zephyr driver return value (`0` or a negative errno) to a Castle status.
 * @see The file documentation for the mapping table.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::status to_status(int result) CASTLE_NOEXCEPT
{
    if (result >= 0)
    {
        return castle::status::ok;
    }
    if (result == -EAGAIN || result == -ENOSPC)
    {
        return castle::status::full;
    }
    if (result == -EINVAL)
    {
        return castle::status::invalid_argument;
    }
    if (result == -ENOTSUP || result == -ERANGE)
    {
        return castle::status::invalid_config;
    }
    if (result == -ENETDOWN)
    {
        return castle::status::not_configured;
    }
    return castle::status::system_call_error;
}

#if CASTLE_USING_IO_CAN_VIA_ZEPHYR

/** @brief Fills a Zephyr frame from a Castle frame. */
CASTLE_INLINE void to_zephyr(frame CASTLE_CONST& in, struct can_frame& out) CASTLE_NOEXCEPT
{
    out = can_frame();
    out.id = in.id;
    out.dlc = in.dlc;
    out.flags = static_cast<uint8_t>((in.is_extended() ? CAN_FRAME_IDE : 0U) | (in.is_remote() ? CAN_FRAME_RTR : 0U));
    for (size_type i = 0U; i < in.data_length(); ++i)
    {
        out.data[i] = in.data[i];
    }
}

/** @brief Converts a Zephyr frame. Returns `data_loss` for CAN FD frames and DLCs above 8. */
CASTLE_INLINE castle::status from_zephyr(struct can_frame CASTLE_CONST& in, frame& out) CASTLE_NOEXCEPT
{
    if ((in.flags & CAN_FRAME_FDF) != 0U || in.dlc > max_data_length)
    {
        return castle::status::data_loss;
    }

    frame result(in.id, ((in.flags & CAN_FRAME_IDE) != 0U) ? id_format::extended : id_format::standard);
    if ((in.flags & CAN_FRAME_RTR) != 0U)
    {
        (void)result.set_remote(in.dlc);
    }
    else
    {
        (void)result.set_data(in.data, in.dlc);
    }
    out = result;
    return castle::status::ok;
}

/** @brief Converts a Castle filter; `CAN_FILTER_IDE` selects extended identifiers. */
CASTLE_INLINE struct can_filter to_zephyr_filter(filter CASTLE_CONST& in) CASTLE_NOEXCEPT
{
    struct can_filter out = can_filter();
    out.id = in.id;
    out.mask = in.mask;
    out.flags = (in.format == id_format::extended) ? CAN_FILTER_IDE : 0U;
    return out;
}

/** @brief Converts a Castle operating mode to Zephyr mode flags. */
CASTLE_NODISCARD CASTLE_INLINE can_mode_t to_zephyr_mode(can_mode mode) CASTLE_NOEXCEPT
{
    switch (mode)
    {
        case can_mode::loopback:    return CAN_MODE_LOOPBACK;
        case can_mode::listen_only: return CAN_MODE_LISTENONLY;
        default:                    return CAN_MODE_NORMAL;
    }
}

/** @brief Converts a Zephyr controller state. */
CASTLE_NODISCARD CASTLE_INLINE bus_state to_bus_state(enum can_state state) CASTLE_NOEXCEPT
{
    switch (state)
    {
        case CAN_STATE_ERROR_ACTIVE:  return bus_state::error_active;
        case CAN_STATE_ERROR_WARNING: return bus_state::error_warning;
        case CAN_STATE_ERROR_PASSIVE: return bus_state::error_passive;
        case CAN_STATE_BUS_OFF:       return bus_state::bus_off;
        default:                      return bus_state::stopped;
    }
}

/** @brief Converts a Castle timeout to a kernel timeout (`K_NO_WAIT`, `K_MSEC()` or `K_FOREVER`). */
CASTLE_NODISCARD CASTLE_INLINE k_timeout_t to_kernel_timeout(timeout_type timeout) CASTLE_NOEXCEPT
{
    if (is_forever(timeout))
    {
        return K_FOREVER;
    }
    if (timeout.count() <= 0)
    {
        return K_NO_WAIT;
    }
    return K_MSEC((timeout.count() > static_cast<int64_t>(INT32_MAX)) ? INT32_MAX : static_cast<int32_t>(timeout.count()));
}

#endif // CASTLE_USING_IO_CAN_VIA_ZEPHYR

} // namespace native

/**
 * @brief Zephyr CAN controller backend.
 * @tparam RxQueueDepth Frames buffered between the driver callback and `receive()`.
 * @tparam MaxFilters Simultaneously installed filters; keep within the controller's limit
 *         (`can_get_max_filters()`, five for the stock MCP2515 driver).
 */
template <size_type RxQueueDepth = 16U, size_type MaxFilters = 8U>
class basic_backend
{
public:
    /** @brief The controller's devicetree handle, e.g. `DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))`. */
    using endpoint_type = CASTLE_CONST struct device*;

    basic_backend() CASTLE_NOEXCEPT
        : device_(nullptr)
    {
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
        k_sem_init(&rx_ready_, 0U, K_SEM_MAX_LIMIT);
#endif
    }

    /** @brief Unregisters the receive callbacks, which hold a pointer to this object, and stops the controller. */
    ~basic_backend() CASTLE_NOEXCEPT
    {
        (void)close(nullptr);
    }

    basic_backend(basic_backend CASTLE_CONST&) CASTLE_DELETE;
    basic_backend& operator=(basic_backend CASTLE_CONST&) CASTLE_DELETE;
    basic_backend(basic_backend&&) CASTLE_DELETE;
    basic_backend& operator=(basic_backend&&) CASTLE_DELETE;

    /**
     * @brief Stops the controller, applies mode and bit rate, then starts it.
     * @return `ok`; `invalid_argument` for a null device; `invalid_config` if already open or rejected by the
     *         driver; `not_configured` when the device is not ready or Zephyr CAN is unavailable;
     *         otherwise the mapped driver error.
     */
    castle::status open(endpoint_type endpoint, can_config CASTLE_CONST& config, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (endpoint == nullptr)
        {
            return castle::status::invalid_argument;
        }
        if (device_ != nullptr)
        {
            return castle::status::invalid_config;
        }
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
        if (!device_is_ready(endpoint))
        {
            return castle::status::not_configured;
        }

        int result = can_stop(endpoint);
        if (result == -EALREADY)
        {
            result = 0;
        }
        if (result == 0)
        {
            result = can_set_mode(endpoint, native::to_zephyr_mode(config.mode));
        }
        if (result == 0 && config.bitrate != 0U)
        {
            result = can_set_bitrate(endpoint, config.bitrate);
        }
        if (result == 0)
        {
            result = can_start(endpoint);
        }
        if (result != 0)
        {
            return fail(result, system_error);
        }

        filters_.clear();
        rx_queue_.clear();
        k_sem_reset(&rx_ready_);
        rx_overruns_ = 0U;
        tx_failures_ = 0U;
        device_ = endpoint;
        return castle::status::ok;
#else
        (void)config;
        return castle::status::not_configured;
#endif
    }

    /** @brief Removes all filters, stops the controller and releases it. Returns `not_configured` when closed. */
    castle::status close(int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (device_ == nullptr)
        {
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
        remove_all_filters();
        int result = can_stop(device_);
        if (result == -EALREADY)
        {
            result = 0;
        }
        device_ = nullptr;
        rx_queue_.clear();
        k_sem_reset(&rx_ready_);
        return (result == 0) ? castle::status::ok : fail(result, system_error);
#else
        return castle::status::not_configured;
#endif
    }

    /** @brief Returns true while a controller is open. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return device_ != nullptr; }

    /**
     * @brief Queues a frame in a TX mailbox, waiting up to `timeout` for a free one.
     * @return `ok` once queued; `full` on timeout; `not_configured` when the controller is stopped or closed;
     *         `system_call_error` otherwise (e.g. `ENETUNREACH` while bus-off).
     */
    castle::status send(frame CASTLE_CONST& value, timeout_type timeout, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (device_ == nullptr)
        {
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
        struct can_frame native_frame;
        native::to_zephyr(value, native_frame);
        CASTLE_CONST int result = can_send(device_, &native_frame, native::to_kernel_timeout(timeout),
                                           &basic_backend::on_transmitted, this);
        return (result == 0) ? castle::status::ok : fail(result, system_error);
#else
        (void)value;
        (void)timeout;
        return castle::status::not_configured;
#endif
    }

    /** @brief Takes the oldest queued frame, waiting up to `timeout`. Returns `empty` when none arrived. */
    castle::status receive(frame& out, timeout_type timeout, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (device_ == nullptr)
        {
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
        if (k_sem_take(&rx_ready_, native::to_kernel_timeout(timeout)) != 0)
        {
            return castle::status::empty;
        }
        return rx_queue_.pop(out);
#else
        (void)out;
        (void)timeout;
        return castle::status::not_configured;
#endif
    }

    /**
     * @brief Registers a filter with `can_add_rx_filter()`.
     * @return `ok`; `full` when `MaxFilters` filters are installed or the controller has no free filter;
     *         `invalid_config` when the controller cannot express the filter; `not_configured` when closed.
     */
    castle::status add_filter(filter CASTLE_CONST& value, filter_handle& handle, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        handle = invalid_filter;
        if (device_ == nullptr)
        {
            return castle::status::not_configured;
        }
        if (filters_.count() >= MaxFilters)
        {
            return castle::status::full;
        }
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
        CASTLE_CONST struct can_filter native_filter = native::to_zephyr_filter(value);
        CASTLE_CONST int result = can_add_rx_filter(device_, &basic_backend::on_received, this, &native_filter);
        if (result < 0)
        {
            return fail(result, system_error);
        }
        handle = filters_.acquire(result);
        return castle::status::ok;
#else
        (void)value;
        return castle::status::not_configured;
#endif
    }

    /** @brief Unregisters a filter. Returns `not_found` for a handle that is not installed. */
    castle::status remove_filter(filter_handle handle, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (device_ == nullptr)
        {
            return castle::status::not_configured;
        }
        if (!filters_.is_used(handle))
        {
            return castle::status::not_found;
        }
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
        can_remove_rx_filter(device_, filters_.at(handle));
#endif
        (void)filters_.release(handle);
        return castle::status::ok;
    }

    /** @brief Unregisters every filter. */
    castle::status clear_filters(int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (device_ == nullptr)
        {
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
        remove_all_filters();
#endif
        return castle::status::ok;
    }

    /** @brief Reads state and error counters from the driver and the software drop counters. */
    castle::status query_status(bus_status& out, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        out = bus_status();
        if (device_ == nullptr)
        {
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
        enum can_state state = CAN_STATE_STOPPED;
        struct can_bus_err_cnt counters = can_bus_err_cnt();
        CASTLE_CONST int result = can_get_state(device_, &state, &counters);
        if (result != 0)
        {
            return fail(result, system_error);
        }
        out.state = native::to_bus_state(state);
        out.tx_error_counter = counters.tx_err_cnt;
        out.rx_error_counter = counters.rx_err_cnt;
        out.rx_overruns = rx_overruns_.load();
        out.tx_failures = tx_failures_.load();
        return castle::status::ok;
#else
        return castle::status::not_configured;
#endif
    }

private:
    static castle::status fail(int result, int* system_error) CASTLE_NOEXCEPT
    {
        if (system_error != nullptr)
        {
            *system_error = -result;
        }
        return native::to_status(result);
    }

#if CASTLE_USING_IO_CAN_VIA_ZEPHYR

    void remove_all_filters() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxFilters; ++i)
        {
            CASTLE_CONST filter_handle handle = static_cast<filter_handle>(i);
            if (filters_.is_used(handle))
            {
                can_remove_rx_filter(device_, filters_.at(handle));
            }
        }
        filters_.clear();
    }

    /** @brief Driver receive callback: the single producer of `rx_queue_`. */
    static void on_received(CASTLE_CONST struct device*, struct can_frame* native_frame, void* user_data)
    {
        basic_backend* self = static_cast<basic_backend*>(user_data);
        frame converted;
        if (!castle::succeeded(native::from_zephyr(*native_frame, converted)))
        {
            return;
        }
        if (castle::succeeded(self->rx_queue_.push(converted)))
        {
            k_sem_give(&self->rx_ready_);
        }
        else
        {
            ++self->rx_overruns_;
        }
    }

    /** @brief Driver transmit-completion callback. */
    static void on_transmitted(CASTLE_CONST struct device*, int error, void* user_data)
    {
        if (error != 0)
        {
            ++static_cast<basic_backend*>(user_data)->tx_failures_;
        }
    }

    struct k_sem rx_ready_;

#endif // CASTLE_USING_IO_CAN_VIA_ZEPHYR

    CASTLE_CONST struct device* device_;
    detail::slot_table<int, MaxFilters> filters_;
    container::mirrored_ring_buffer<frame, RxQueueDepth> rx_queue_;
    castle::atomic<uint32_t> rx_overruns_;
    castle::atomic<uint32_t> tx_failures_;
};

/** @brief Zephyr backend with a 16-frame receive queue and room for eight filters. */
using backend = basic_backend<>;

} // namespace zephyr
} // namespace can
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_CAN_BACKENDS_ZEPHYR_HPP
