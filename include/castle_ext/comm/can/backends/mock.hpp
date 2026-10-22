// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mock.hpp
 * @brief Deterministic in-memory CAN backend for host-side samples and tests.
 *
 * `mock::basic_backend` behaves like a single CAN node whose wire is under test control:
 *
 * - Frames passed to `send()` land in a bounded transmit queue read back with `pop_sent()`
 *   (or, in `can_mode::loopback`, are delivered straight to the node's own receive path).
 * - `inject()` plays the part of the bus delivering a frame: it is accepted only when an
 *   installed filter matches, exactly like a hardware acceptance filter.
 * - `fail_next_send()` and `set_bus_state()` script faults.
 *
 * The mock never blocks: timeouts are accepted and ignored. It is not thread safe.
 */
#ifndef CASTLE_EXT_COMM_CAN_BACKENDS_MOCK_HPP
#define CASTLE_EXT_COMM_CAN_BACKENDS_MOCK_HPP

#include "castle/container/mirrored_ring_buffer.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/comm/can/detail/slot_table.hpp"
#include "castle_ext/comm/can/filter.hpp"
#include "castle_ext/comm/can/frame.hpp"
#include "castle_ext/comm/can/types.hpp"

#include <errno.h>
#include <stdint.h>

namespace castle
{
namespace comm
{
namespace can
{
namespace mock
{

/**
 * @brief Host-side CAN controller double.
 * @tparam RxQueueDepth Frames buffered between `inject()`/loopback and `receive()`.
 * @tparam TxQueueDepth Frames buffered between `send()` and `pop_sent()`.
 * @tparam MaxFilters Simultaneously installed filters.
 */
template <size_type RxQueueDepth = 16U, size_type TxQueueDepth = 16U, size_type MaxFilters = 8U>
class basic_backend
{
public:
    /** @brief The mock names its virtual interface with a non-empty string, like SocketCAN does. */
    using endpoint_type = CASTLE_CONST char*;

    basic_backend() CASTLE_NOEXCEPT
        : config_()
        , state_(bus_state::stopped)
        , tx_error_counter_(0U)
        , rx_error_counter_(0U)
        , rx_overruns_(0U)
        , tx_failures_(0U)
        , forced_send_status_(castle::status::ok)
        , open_(false)
    {
    }

    /**
     * @brief Opens the virtual controller.
     * @return `ok`, `invalid_argument` for a null/empty name, or `invalid_config` when already open.
     */
    castle::status open(endpoint_type endpoint, can_config CASTLE_CONST& config, int* system_error) CASTLE_NOEXCEPT
    {
        reset_error(system_error);
        if (endpoint == nullptr || endpoint[0] == '\0')
        {
            return castle::status::invalid_argument;
        }
        if (open_)
        {
            return castle::status::invalid_config;
        }

        config_ = config;
        state_ = bus_state::error_active;
        tx_error_counter_ = 0U;
        rx_error_counter_ = 0U;
        rx_overruns_ = 0U;
        tx_failures_ = 0U;
        forced_send_status_ = castle::status::ok;
        filters_.clear();
        rx_queue_.clear();
        tx_queue_.clear();
        open_ = true;
        return castle::status::ok;
    }

    /** @brief Closes the controller and discards queued frames. Returns `not_configured` if already closed. */
    castle::status close(int* system_error) CASTLE_NOEXCEPT
    {
        reset_error(system_error);
        if (!open_)
        {
            return castle::status::not_configured;
        }
        open_ = false;
        state_ = bus_state::stopped;
        filters_.clear();
        rx_queue_.clear();
        tx_queue_.clear();
        return castle::status::ok;
    }

    /** @brief Returns true while the virtual controller is open. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return open_; }

    /**
     * @brief Queues a frame for transmission (or loops it back in `can_mode::loopback`).
     * @return `ok`; `full` when the transmit queue holds `TxQueueDepth` unread frames;
     *         `invalid_config` in `listen_only` mode; `system_call_error` while bus-off (errno `ENETUNREACH`);
     *         the status armed by `fail_next_send()`; `not_configured` when closed.
     */
    castle::status send(frame CASTLE_CONST& value, timeout_type, int* system_error) CASTLE_NOEXCEPT
    {
        reset_error(system_error);
        if (!open_)
        {
            return castle::status::not_configured;
        }
        if (config_.mode == can_mode::listen_only)
        {
            return castle::status::invalid_config;
        }
        if (forced_send_status_ != castle::status::ok)
        {
            CASTLE_CONST castle::status forced = forced_send_status_;
            forced_send_status_ = castle::status::ok;
            ++tx_failures_;
            return forced;
        }
        if (state_ == bus_state::bus_off)
        {
            ++tx_failures_;
            if (system_error != nullptr)
            {
                *system_error = ENETUNREACH;
            }
            return castle::status::system_call_error;
        }

        if (config_.mode == can_mode::loopback)
        {
            (void)deliver(value);
            return castle::status::ok;
        }
        return tx_queue_.push(value);
    }

    /** @brief Takes the oldest delivered frame. Returns `empty` immediately when there is none. */
    castle::status receive(frame& out, timeout_type, int* system_error) CASTLE_NOEXCEPT
    {
        reset_error(system_error);
        if (!open_)
        {
            return castle::status::not_configured;
        }
        return rx_queue_.pop(out);
    }

    /** @brief Installs a filter. Returns `full` when `MaxFilters` filters are installed. */
    castle::status add_filter(filter CASTLE_CONST& value, filter_handle& handle, int* system_error) CASTLE_NOEXCEPT
    {
        reset_error(system_error);
        handle = invalid_filter;
        if (!open_)
        {
            return castle::status::not_configured;
        }
        handle = filters_.acquire(value);
        return (handle == invalid_filter) ? castle::status::full : castle::status::ok;
    }

    /** @brief Removes a filter. Returns `not_found` for a handle that is not installed. */
    castle::status remove_filter(filter_handle handle, int* system_error) CASTLE_NOEXCEPT
    {
        reset_error(system_error);
        if (!open_)
        {
            return castle::status::not_configured;
        }
        return filters_.release(handle) ? castle::status::ok : castle::status::not_found;
    }

    /** @brief Removes every filter. */
    castle::status clear_filters(int* system_error) CASTLE_NOEXCEPT
    {
        reset_error(system_error);
        if (!open_)
        {
            return castle::status::not_configured;
        }
        filters_.clear();
        return castle::status::ok;
    }

    /** @brief Reports the scripted bus state, error counters and drop counters. */
    castle::status query_status(bus_status& out, int* system_error) CASTLE_NOEXCEPT
    {
        reset_error(system_error);
        out = bus_status();
        if (!open_)
        {
            return castle::status::not_configured;
        }
        out.state = state_;
        out.tx_error_counter = tx_error_counter_;
        out.rx_error_counter = rx_error_counter_;
        out.rx_overruns = rx_overruns_;
        out.tx_failures = tx_failures_;
        return castle::status::ok;
    }

    /**
     * @brief Plays the bus delivering `value` to this node.
     * @return `ok` when buffered for `receive()`; `not_found` when no installed filter matches;
     *         `full` when the receive queue overflowed (counted in `rx_overruns`); `not_configured` when closed.
     */
    castle::status inject(frame CASTLE_CONST& value) CASTLE_NOEXCEPT
    {
        return open_ ? deliver(value) : castle::status::not_configured;
    }

    /** @brief Takes the oldest frame the node transmitted. Returns `empty` when there is none. */
    castle::status pop_sent(frame& out) CASTLE_NOEXCEPT
    {
        return tx_queue_.pop(out);
    }

    /** @brief Returns the number of transmitted frames waiting for `pop_sent()`. */
    CASTLE_NODISCARD size_type sent_count() CASTLE_CONST CASTLE_NOEXCEPT { return tx_queue_.size(); }

    /** @brief Makes the next `send()` fail with `failure` (one shot). `status::ok` disarms it. */
    void fail_next_send(castle::status failure) CASTLE_NOEXCEPT { forced_send_status_ = failure; }

    /** @brief Scripts the state and error counters returned by `query_status()`. */
    void set_bus_state(bus_state state, uint8_t tx_errors = 0U, uint8_t rx_errors = 0U) CASTLE_NOEXCEPT
    {
        state_ = state;
        tx_error_counter_ = tx_errors;
        rx_error_counter_ = rx_errors;
    }

    /** @brief Returns the configuration passed to the last successful `open()`. */
    CASTLE_NODISCARD can_config CASTLE_CONST& configuration() CASTLE_CONST CASTLE_NOEXCEPT { return config_; }

    /** @brief Returns the number of currently installed filters. */
    CASTLE_NODISCARD size_type filter_count() CASTLE_CONST CASTLE_NOEXCEPT { return filters_.count(); }

private:
    static void reset_error(int* system_error) CASTLE_NOEXCEPT
    {
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
    }

    castle::status deliver(frame CASTLE_CONST& value) CASTLE_NOEXCEPT
    {
        bool accepted = false;
        for (size_type i = 0U; i < MaxFilters && !accepted; ++i)
        {
            accepted = filters_.is_used(static_cast<filter_handle>(i))
                    && filters_.at(static_cast<filter_handle>(i)).matches(value);
        }
        if (!accepted)
        {
            return castle::status::not_found;
        }
        if (!castle::succeeded(rx_queue_.push(value)))
        {
            ++rx_overruns_;
            return castle::status::full;
        }
        return castle::status::ok;
    }

    can_config config_;
    bus_state state_;
    uint8_t tx_error_counter_;
    uint8_t rx_error_counter_;
    uint32_t rx_overruns_;
    uint32_t tx_failures_;
    castle::status forced_send_status_;
    bool open_;
    detail::slot_table<filter, MaxFilters> filters_;
    container::mirrored_ring_buffer<frame, RxQueueDepth> rx_queue_;
    container::mirrored_ring_buffer<frame, TxQueueDepth> tx_queue_;
};

/** @brief Mock backend with default queue depths (16 receive frames, 16 transmit frames, 8 filters). */
using backend = basic_backend<>;

} // namespace mock
} // namespace can
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_CAN_BACKENDS_MOCK_HPP
