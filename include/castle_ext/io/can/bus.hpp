// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bus.hpp
 * @brief Backend-independent CAN bus facade with compile-time backend selection.
 *
 * `basic_bus<Backend>` owns one backend object and forwards every call to it by direct, inlinable
 * member calls: no virtual functions, no RTTI, no heap. The facade adds what every backend would
 * otherwise repeat: argument validation (`validate_config`, `validate_frame`, `validate_filter`,
 * non-negative timeouts), the optional-`errno` convention of Castle's `io` components, and the
 * convenience calls `send(id, bytes, ...)` and `accept_all()`.
 *
 * Backend contract (checked by a single `static_assert`):
 *
 * @code
 * class backend {
 * public:
 *     using endpoint_type = ...; // how the platform names a controller: "can0", const device*, ...
 *     castle::status open(endpoint_type, can_config const&, int* system_error);
 *     castle::status close(int* system_error);
 *     bool is_open() const;
 *     castle::status send(frame const&, timeout_type, int* system_error);
 *     castle::status receive(frame&, timeout_type, int* system_error);
 *     castle::status add_filter(filter const&, filter_handle&, int* system_error);
 *     castle::status remove_filter(filter_handle, int* system_error);
 *     castle::status clear_filters(int* system_error);
 *     castle::status query_status(bus_status&, int* system_error);
 * };
 * @endcode
 *
 * Backend semantics shared by all platforms:
 * - After `open()` no filter is installed and `receive()` yields nothing until `add_filter()` or
 *   `accept_all()` is called (Zephyr works this way natively; the other backends match it).
 * - `send()` returns once the frame is queued for transmission; `full` means the queue stayed
 *   full for the whole timeout. `receive()` returns `empty` when no frame arrived in time.
 * - Timeouts are non-negative; `no_wait` never blocks and `wait_forever` blocks indefinitely.
 *
 * Thread safety is delegated to the backend (see its documentation); the facade adds no locks.
 *
 * @code
 * castle::io::can::default_bus bus;   // SocketCAN on Linux, Zephyr CAN on Zephyr
 * bus.open(endpoint, castle::io::can::can_config(castle::io::can::bitrate_500k));
 * bus.accept_all();
 * bus.send(0x123U, payload, 8U);
 * castle::io::can::frame rx;
 * if (bus.receive(rx, castle::chrono::milliseconds(100)) == castle::status::ok) { ... }
 * @endcode
 */
#ifndef CASTLE_EXT_IO_CAN_BUS_HPP
#define CASTLE_EXT_IO_CAN_BUS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/io/can/filter.hpp"
#include "castle_ext/io/can/frame.hpp"
#include "castle_ext/io/can/types.hpp"
#include "castle_ext/io/fd/control.hpp"

namespace castle
{
namespace io
{
namespace can
{

namespace detail
{

template <typename Backend, typename = void>
struct has_bus_backend_contract : meta::false_type {};

/**
 * @brief Detects whether `Backend` implements the `basic_bus` backend contract.
 * @tparam Backend Candidate platform backend.
 */
template <typename Backend>
struct has_bus_backend_contract<Backend, meta::void_t<
    typename Backend::endpoint_type,
    decltype(meta::declval<Backend&>().open(
        meta::declval<typename Backend::endpoint_type>(),
        meta::declval<can_config CASTLE_CONST&>(),
        meta::declval<int*>())),
    decltype(meta::declval<Backend&>().close(meta::declval<int*>())),
    decltype(meta::declval<Backend CASTLE_CONST&>().is_open()),
    decltype(meta::declval<Backend&>().send(
        meta::declval<frame CASTLE_CONST&>(), meta::declval<timeout_type>(), meta::declval<int*>())),
    decltype(meta::declval<Backend&>().receive(
        meta::declval<frame&>(), meta::declval<timeout_type>(), meta::declval<int*>())),
    decltype(meta::declval<Backend&>().add_filter(
        meta::declval<filter CASTLE_CONST&>(), meta::declval<filter_handle&>(), meta::declval<int*>())),
    decltype(meta::declval<Backend&>().remove_filter(meta::declval<filter_handle>(), meta::declval<int*>())),
    decltype(meta::declval<Backend&>().clear_filters(meta::declval<int*>())),
    decltype(meta::declval<Backend&>().query_status(meta::declval<bus_status&>(), meta::declval<int*>()))>>
    : meta::true_type {};

} // namespace detail

/**
 * @brief CAN bus facade over a compile-time selected backend.
 * @tparam Backend Platform backend implementing the contract in the file documentation.
 */
template <typename Backend>
class basic_bus
{
public:
    using backend_type  = Backend;
    using endpoint_type = typename Backend::endpoint_type;

    static_assert(detail::has_bus_backend_contract<Backend>::value,
                  "Backend must implement the basic_bus backend contract: endpoint_type plus "
                  "open/close/is_open/send/receive/add_filter/remove_filter/clear_filters/query_status "
                  "(see the bus.hpp file documentation).");

    basic_bus() CASTLE_DEFAULT;
    basic_bus(basic_bus CASTLE_CONST&) CASTLE_DELETE;
    basic_bus& operator=(basic_bus CASTLE_CONST&) CASTLE_DELETE;
    basic_bus(basic_bus&&) CASTLE_DEFAULT;
    basic_bus& operator=(basic_bus&&) CASTLE_DEFAULT;

    /**
     * @brief Opens a controller, applies the configuration and starts it.
     * @param endpoint Platform name of the controller (`backend_type::endpoint_type`).
     * @param config Bit rate and operating mode.
     * @param system_error Optional output receiving the platform error number on failure.
     * @return `ok`, `invalid_config`, `invalid_argument`, `system_call_error` or `not_configured`.
     */
    CASTLE_NODISCARD castle::status open(endpoint_type endpoint,
                        can_config CASTLE_CONST& config = can_config(),
                        int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        CASTLE_CONST castle::status valid = validate_config(config);
        if (!castle::succeeded(valid))
        {
            return valid;
        }
        return backend_.open(endpoint, config, system_error);
    }

    /** @brief Removes all filters, stops the controller and releases it. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        return backend_.close(system_error);
    }

    /** @brief Returns true while a controller is open. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return backend_.is_open();
    }

    /**
     * @brief Queues a frame for transmission.
     * @param value Frame to send; checked with `validate_frame()`.
     * @param timeout How long to wait for transmit space; non-negative.
     * @param system_error Optional output receiving the platform error number on failure.
     * @return `ok` once queued; `full` if no space became available in time; `invalid_argument`;
     *         `not_configured` when closed; `system_call_error` otherwise.
     */
    CASTLE_NODISCARD castle::status send(frame CASTLE_CONST& value,
                        timeout_type timeout = no_wait,
                        int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (!castle::succeeded(validate_frame(value)) || timeout.count() < 0)
        {
            return castle::status::invalid_argument;
        }
        return backend_.send(value, timeout, system_error);
    }

    /**
     * @brief Builds and queues a data frame.
     * @param id Frame identifier, checked against `format`.
     * @param bytes Payload; may be null only when `length` is zero.
     * @param length Payload length, at most 8.
     * @param format Identifier format.
     * @return The status of `frame::set_data()` or of `send()`.
     */
    CASTLE_NODISCARD castle::status send(uint32_t id,
                        CASTLE_CONST void* bytes,
                        size_type length,
                        id_format format = id_format::standard,
                        timeout_type timeout = no_wait,
                        int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        frame value(id, format);
        CASTLE_CONST castle::status filled = value.set_data(bytes, length);
        if (!castle::succeeded(filled))
        {
            castle::io::fd::detail::clear_system_error(system_error);
            return filled;
        }
        return send(value, timeout, system_error);
    }

    /**
     * @brief Takes the oldest received frame that passed an installed filter.
     * @param out Receives the frame; unchanged unless `ok` is returned.
     * @param timeout How long to wait for a frame; non-negative.
     * @return `ok`; `empty` when nothing arrived in time; `invalid_argument`; `not_configured`
     *         when closed; `system_call_error`; `data_loss` for a truncated kernel frame.
     */
    CASTLE_NODISCARD castle::status receive(frame& out,
                           timeout_type timeout = no_wait,
                           int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (timeout.count() < 0)
        {
            return castle::status::invalid_argument;
        }
        return backend_.receive(out, timeout, system_error);
    }

    /**
     * @brief Installs a receive filter.
     * @param value Filter to install; checked with `validate_filter()`.
     * @param handle Optional output receiving the handle for `remove_filter()`.
     * @return `ok`; `full` when the backend has no free filter slot; `invalid_argument`;
     *         `not_configured` when closed; `invalid_config` when the hardware rejects the filter.
     */
    CASTLE_NODISCARD castle::status add_filter(filter CASTLE_CONST& value,
                              filter_handle* handle = nullptr,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (handle != nullptr)
        {
            *handle = invalid_filter;
        }
        CASTLE_CONST castle::status valid = validate_filter(value);
        if (!castle::succeeded(valid))
        {
            return valid;
        }

        filter_handle installed = invalid_filter;
        CASTLE_CONST castle::status result = backend_.add_filter(value, installed, system_error);
        if (castle::succeeded(result) && handle != nullptr)
        {
            *handle = installed;
        }
        return result;
    }

    /**
     * @brief Installs one catch-all filter per identifier format (two filter slots).
     * @note Clear them with `clear_filters()`; the handles are not returned.
     */
    CASTLE_NODISCARD castle::status accept_all(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        filter_handle standard_handle = invalid_filter;
        castle::status result = add_filter(filter::any(id_format::standard), &standard_handle, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
        result = add_filter(filter::any(id_format::extended), nullptr, system_error);
        if (!castle::succeeded(result))
        {
            (void)backend_.remove_filter(standard_handle, nullptr);
        }
        return result;
    }

    /** @brief Removes one filter. Returns `not_found` for a handle that is not installed. */
    castle::status remove_filter(filter_handle handle, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        return backend_.remove_filter(handle, system_error);
    }

    /** @brief Removes every filter; `receive()` yields nothing afterwards. */
    castle::status clear_filters(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        return backend_.clear_filters(system_error);
    }

    /** @brief Reads the controller state, error counters and drop counters. */
    CASTLE_NODISCARD castle::status query_status(bus_status& out, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        return backend_.query_status(out, system_error);
    }

    /** @brief Returns the backend for platform-specific access (e.g. the SocketCAN descriptor). */
    CASTLE_NODISCARD backend_type& backend() CASTLE_NOEXCEPT { return backend_; }

    /** @brief Returns the backend for platform-specific access. */
    CASTLE_NODISCARD backend_type CASTLE_CONST& backend() CASTLE_CONST CASTLE_NOEXCEPT { return backend_; }

private:
    backend_type backend_;
};

/** @brief Convenience alias mirroring `castle::io::gpio::pin`. */
template <typename Backend>
using bus = basic_bus<Backend>;

} // namespace can
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_CAN_BUS_HPP
