// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file socket_can.hpp
 * @brief Linux SocketCAN backend over a raw `PF_CAN`/`CAN_RAW` socket descriptor.
 *
 * The backend creates the socket, resolves the interface with `SIOCGIFINDEX`, binds a
 * `struct sockaddr_can`, switches the descriptor to non-blocking mode with `fcntl()` and then
 * works directly on the descriptor (`read`/`write`/`poll`) through Castle's `fd` helpers. Frames
 * are exchanged as classic `struct can_frame`; the kernel's error frames are decoded into the
 * controller state and counters reported by `query_status()`.
 *
 * An MCP2515 on SPI appears to this backend as an ordinary interface (`can0`) once the kernel's
 * `mcp251x` driver is bound through the device tree. Bit timing belongs to the interface:
 * `ip link set can0 up type can bitrate 500000`. The `bitrate` of `can_config` is not applied.
 *
 * Mapping of the portable contract:
 * - `endpoint_type` is the interface name (`"can0"`, `"vcan0"`); `"any"` binds to every CAN
 *   interface for receiving (sending then fails because the interface is unspecified).
 * - `can_mode::loopback` enables `CAN_RAW_RECV_OWN_MSGS`; `can_mode::listen_only` is an
 *   interface property (`ip link ... listen-only on`) and is rejected with `invalid_config`.
 * - Filters are held in a fixed table and applied with `CAN_RAW_FILTER`. A freshly opened
 *   socket has an empty list and therefore receives nothing until a filter is added.
 * - `send()` returns `full` when the socket buffer (`EAGAIN`) or the interface queue (`ENOBUFS`)
 *   is full. A timeout covers the `EAGAIN` back-pressure only, with one retry after the wait.
 * - Error frames for `TX_TIMEOUT`, `CRTL`, `BUSOFF` and `RESTARTED` are requested with
 *   `CAN_RAW_ERR_FILTER` and consumed inside `receive()`, so state and counters are current as
 *   of the last `receive()` call.
 *
 * One thread may send while another receives; concurrent calls of the same kind and filter
 * changes during I/O need external synchronization. On other targets every operation returns
 * `status::not_configured`.
 */
#ifndef CASTLE_EXT_IO_CAN_BACKENDS_SOCKET_CAN_HPP
#define CASTLE_EXT_IO_CAN_BACKENDS_SOCKET_CAN_HPP

#include "castle/chrono/clocks.hpp"
#include "castle/chrono/duration.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/move.hpp"
#include "castle_ext/io/can/detail/slot_table.hpp"
#include "castle_ext/io/can/filter.hpp"
#include "castle_ext/io/can/frame.hpp"
#include "castle_ext/io/can/types.hpp"
#include "castle_ext/io/fd/control.hpp"
#include "castle_ext/io/fd/descriptor.hpp"

#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
    #include "castle_ext/io/fd/poll.hpp"

    #include <errno.h>
    #include <limits.h>
    #include <net/if.h>
    #include <string.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <linux/can.h>
    #include <linux/can/error.h>
    #include <linux/can/raw.h>
#endif

namespace castle
{
namespace io
{
namespace can
{
namespace socket_can
{

/** @brief True when the Linux SocketCAN UAPI is available in this build. */
static CASTLE_INLINE CASTLE_CONSTEXPR bool socket_can_available = (CASTLE_USING_IO_CAN_VIA_SOCKET_CAN != 0);

#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN

/**
 * @brief Translation between Castle CAN types and the kernel's SocketCAN structures.
 *
 * These pure functions hold all knowledge of the kernel ABI, so they can be verified without
 * a CAN interface.
 */
namespace native
{

/** @brief Error-frame classes requested from the kernel with `CAN_RAW_ERR_FILTER`. */
static CASTLE_INLINE CASTLE_CONSTEXPR can_err_mask_t error_mask =
    CAN_ERR_TX_TIMEOUT | CAN_ERR_CRTL | CAN_ERR_BUSOFF | CAN_ERR_RESTARTED;

/** @brief Fills a kernel frame from a Castle frame (identifier flags, DLC and zero-padded payload). */
CASTLE_INLINE void to_kernel(frame CASTLE_CONST& in, struct can_frame& out) CASTLE_NOEXCEPT
{
    out = can_frame();
    out.can_id = in.id & (in.is_extended() ? CAN_EFF_MASK : CAN_SFF_MASK);
    if (in.is_extended())
    {
        out.can_id |= CAN_EFF_FLAG;
    }
    if (in.is_remote())
    {
        out.can_id |= CAN_RTR_FLAG;
    }
    out.can_dlc = in.dlc;
    for (size_type i = 0U; i < max_data_length; ++i)
    {
        out.data[i] = in.data[i];
    }
}

/**
 * @brief Converts a kernel data or remote frame.
 * @return `ok`, or `data_loss` for an error frame or a DLC the classic format cannot carry.
 */
CASTLE_INLINE castle::status from_kernel(struct can_frame CASTLE_CONST& in, frame& out) CASTLE_NOEXCEPT
{
    if ((in.can_id & CAN_ERR_FLAG) != 0U || in.can_dlc > max_data_length)
    {
        return castle::status::data_loss;
    }

    CASTLE_CONST bool extended = (in.can_id & CAN_EFF_FLAG) != 0U;
    frame result(in.can_id & (extended ? CAN_EFF_MASK : CAN_SFF_MASK),
                 extended ? id_format::extended : id_format::standard);
    if ((in.can_id & CAN_RTR_FLAG) != 0U)
    {
        (void)result.set_remote(in.can_dlc);
    }
    else
    {
        (void)result.set_data(in.data, in.can_dlc);
    }
    out = result;
    return castle::status::ok;
}

/** @brief Converts a Castle filter. The extended-format flag is always part of the mask, so formats never mix. */
CASTLE_INLINE struct can_filter to_kernel_filter(filter CASTLE_CONST& in) CASTLE_NOEXCEPT
{
    CASTLE_CONST bool extended = in.format == id_format::extended;
    CASTLE_CONST canid_t id_bits = extended ? CAN_EFF_MASK : CAN_SFF_MASK;
    struct can_filter out;
    out.can_id = (in.id & id_bits) | (extended ? CAN_EFF_FLAG : 0U);
    out.can_mask = (in.mask & id_bits) | CAN_EFF_FLAG;
    return out;
}

/**
 * @brief Folds a kernel error frame into the controller state and counters.
 * @param in Frame with `CAN_ERR_FLAG` set.
 * @param status Receives the update: state from `BUSOFF`/`RESTARTED`/`CRTL`, overflow and
 *        timeout counts, and the controller error counters when the kernel reports them.
 */
CASTLE_INLINE void apply_error_frame(struct can_frame CASTLE_CONST& in, bus_status& status) CASTLE_NOEXCEPT
{
    if ((in.can_id & CAN_ERR_TX_TIMEOUT) != 0U)
    {
        ++status.tx_failures;
    }
    if ((in.can_id & CAN_ERR_RESTARTED) != 0U)
    {
        status.state = bus_state::error_active;
    }
    if ((in.can_id & CAN_ERR_BUSOFF) != 0U)
    {
        status.state = bus_state::bus_off;
    }
    if ((in.can_id & CAN_ERR_CRTL) != 0U)
    {
        CASTLE_CONST uint8_t problem = in.data[1];
        if ((problem & CAN_ERR_CRTL_RX_OVERFLOW) != 0U)
        {
            ++status.rx_overruns;
        }
        if ((problem & CAN_ERR_CRTL_TX_OVERFLOW) != 0U)
        {
            ++status.tx_failures;
        }
        if ((problem & (CAN_ERR_CRTL_RX_PASSIVE | CAN_ERR_CRTL_TX_PASSIVE)) != 0U)
        {
            status.state = bus_state::error_passive;
        }
        else if ((problem & (CAN_ERR_CRTL_RX_WARNING | CAN_ERR_CRTL_TX_WARNING)) != 0U)
        {
            status.state = bus_state::error_warning;
        }
        else if ((problem & CAN_ERR_CRTL_ACTIVE) != 0U)
        {
            status.state = bus_state::error_active;
        }
#if defined(CAN_ERR_CNT)
        if ((in.can_id & CAN_ERR_CNT) != 0U)
#endif
        {
            status.tx_error_counter = in.data[6];
            status.rx_error_counter = in.data[7];
        }
    }
}

} // namespace native

#endif // CASTLE_USING_IO_CAN_VIA_SOCKET_CAN

/**
 * @brief SocketCAN controller backend.
 * @tparam MaxFilters Simultaneously installed filters (the kernel accepts up to `CAN_RAW_FILTER_MAX`).
 */
template <size_type MaxFilters = 8U>
class basic_backend
{
public:
    /** @brief Interface name such as `"can0"`, or `"any"` for a receive-only bind to all CAN interfaces. */
    using endpoint_type = CASTLE_CONST char*;

    basic_backend() CASTLE_DEFAULT;

    /**
     * @brief Creates the socket, binds it to the interface and starts with an empty filter list.
     * @return `ok`; `invalid_argument` for a null, empty or over-long name; `invalid_config` if already
     *         open or for `listen_only`; `system_call_error` (errno in `system_error`, e.g. `ENODEV` for an
     *         unknown interface); `not_configured` when SocketCAN is unavailable.
     */
    castle::status open(endpoint_type endpoint, can_config CASTLE_CONST& config, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (endpoint == nullptr || endpoint[0] == '\0')
        {
            return castle::status::invalid_argument;
        }
#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
        if (strnlen(endpoint, IFNAMSIZ) >= static_cast<size_t>(IFNAMSIZ))
        {
            return castle::status::invalid_argument;
        }
        if (descriptor_.is_open() || config.mode == can_mode::listen_only)
        {
            return castle::status::invalid_config;
        }

        CASTLE_CONST int raw_socket = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (raw_socket < 0)
        {
            castle::io::fd::detail::capture_system_error(system_error);
            return castle::status::system_call_error;
        }
        castle::io::fd::basic_fd next(raw_socket);

        castle::status result = prepare_descriptor(next, system_error);
        if (castle::succeeded(result))
        {
            result = bind_interface(raw_socket, endpoint, system_error);
        }
        if (castle::succeeded(result))
        {
            result = set_option(raw_socket, CAN_RAW_FILTER, nullptr, 0U, system_error);
        }
        if (castle::succeeded(result))
        {
            CASTLE_CONST can_err_mask_t errors = native::error_mask;
            result = set_option(raw_socket, CAN_RAW_ERR_FILTER, &errors, sizeof(errors), system_error);
        }
        if (castle::succeeded(result))
        {
            CASTLE_CONST int own_messages = (config.mode == can_mode::loopback) ? 1 : 0;
            result = set_option(raw_socket, CAN_RAW_RECV_OWN_MSGS, &own_messages, sizeof(own_messages), system_error);
        }
        if (!castle::succeeded(result))
        {
            return result;
        }

        descriptor_ = CASTLE_MOVE(next);
        start_session();
        return castle::status::ok;
#else
        (void)config;
        return castle::status::not_configured;
#endif
    }

    /**
     * @brief Takes ownership of an already created and bound CAN_RAW socket.
     * @param descriptor_value Socket descriptor; ownership transfers to the backend even on failure.
     * @return `ok`, `invalid_argument` for a negative descriptor, `invalid_config` if already open,
     *         or the `fcntl()` failure. Filters and options set by the caller stay untouched.
     * @note The adopted socket starts with an empty tracked filter table; options such as filters
     *       installed earlier on the descriptor itself remain in force.
     */
    castle::status adopt(int descriptor_value, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        castle::io::fd::basic_fd next(descriptor_value);
        if (!next.is_open())
        {
            return castle::status::invalid_argument;
        }
        if (descriptor_.is_open())
        {
            return castle::status::invalid_config;
        }

        CASTLE_CONST castle::status result = prepare_descriptor(next, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
        descriptor_ = CASTLE_MOVE(next);
        start_session();
        return castle::status::ok;
    }

    /** @brief Closes the socket and forgets all filters. Returns `not_configured` when already closed. */
    castle::status close(int* system_error) CASTLE_NOEXCEPT
    {
        status_.state = bus_state::stopped;
        filters_.clear();
        return descriptor_.close(system_error);
    }

    /** @brief Returns true while the socket is open. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.is_open(); }

    /** @brief Returns the socket descriptor, or -1 when closed. */
    CASTLE_NODISCARD int native_handle() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.native_handle(); }

    /**
     * @brief Writes one frame to the socket.
     * @return `ok`; `full` when the socket buffer or interface queue stayed full for the timeout;
     *         `system_call_error` (errno in `system_error`); `not_configured` when closed.
     */
    castle::status send(frame CASTLE_CONST& value, timeout_type timeout, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (!descriptor_.is_open())
        {
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
        struct can_frame raw;
        native::to_kernel(value, raw);

        castle::status result = write_frame(raw, system_error);
        if (result == castle::status::full && timeout.count() > 0)
        {
            short ready = 0;
            result = castle::io::fd::wait_fd(descriptor_.native_handle(), castle::io::fd::poll_write, to_poll_timeout(timeout), ready, system_error);
            result = castle::succeeded(result) ? write_frame(raw, system_error)
                   : (result == castle::status::time_out ? castle::status::full : result);
        }
        return result;
#else
        (void)value;
        (void)timeout;
        return castle::status::not_configured;
#endif
    }

    /**
     * @brief Reads the next frame, consuming kernel error frames on the way.
     * @return `ok`; `empty` when nothing arrived within the timeout; `data_loss` for a truncated or
     *         unrepresentable kernel frame; `system_call_error` (e.g. `ENETDOWN`); `not_configured` when closed.
     */
    castle::status receive(frame& out, timeout_type timeout, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (!descriptor_.is_open())
        {
            return castle::status::not_configured;
        }
#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
        CASTLE_CONST bool forever = is_forever(timeout);
        CASTLE_CONST castle::chrono::steady_clock::time_point deadline =
            forever ? castle::chrono::steady_clock::time_point() : castle::chrono::steady_clock::now() + timeout;

        for (;;)
        {
            struct can_frame raw;
            size_type count = 0U;
            castle::status result = castle::io::fd::read_fd(descriptor_.native_handle(), &raw, sizeof(raw), count, system_error);
            if (castle::succeeded(result))
            {
                if (count != sizeof(raw))
                {
                    return castle::status::data_loss;
                }
                if ((raw.can_id & CAN_ERR_FLAG) == 0U)
                {
                    return native::from_kernel(raw, out);
                }
                native::apply_error_frame(raw, status_);
                continue;
            }
            if (result != castle::status::empty)
            {
                return result;
            }

            castle::io::fd::detail::clear_system_error(system_error); // EAGAIN just means "nothing queued yet"
            CASTLE_CONST int wait_ms = forever ? -1 : remaining_ms(deadline);
            if (wait_ms == 0)
            {
                return castle::status::empty;
            }
            short ready = 0;
            result = castle::io::fd::wait_fd(descriptor_.native_handle(), castle::io::fd::poll_read, wait_ms, ready, system_error);
            if (!castle::succeeded(result))
            {
                return (result == castle::status::time_out) ? castle::status::empty : result;
            }
        }
#else
        (void)out;
        (void)timeout;
        return castle::status::not_configured;
#endif
    }

    /**
     * @brief Installs a filter and re-applies the filter list with `CAN_RAW_FILTER`.
     * @return `ok`; `full` when `MaxFilters` filters are installed; `system_call_error` if the kernel rejects
     *         the list (the filter is not kept); `not_configured` when closed.
     */
    castle::status add_filter(filter CASTLE_CONST& value, filter_handle& handle, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        handle = invalid_filter;
        if (!descriptor_.is_open())
        {
            return castle::status::not_configured;
        }

        CASTLE_CONST filter_handle slot = filters_.acquire(value);
        if (slot == invalid_filter)
        {
            return castle::status::full;
        }
        CASTLE_CONST castle::status result = apply_filters(system_error);
        if (!castle::succeeded(result))
        {
            (void)filters_.release(slot);
            return result;
        }
        handle = slot;
        return castle::status::ok;
    }

    /** @brief Removes a filter. Returns `not_found` for a handle that is not installed. */
    castle::status remove_filter(filter_handle handle, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (!descriptor_.is_open())
        {
            return castle::status::not_configured;
        }
        if (!filters_.is_used(handle))
        {
            return castle::status::not_found;
        }

        filter saved = filters_.at(handle);
        (void)filters_.release(handle);
        CASTLE_CONST castle::status result = apply_filters(system_error);
        if (!castle::succeeded(result))
        {
            (void)filters_.acquire(saved);
        }
        return result;
    }

    /** @brief Removes every filter; the socket then receives nothing. */
    castle::status clear_filters(int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        if (!descriptor_.is_open())
        {
            return castle::status::not_configured;
        }
        filters_.clear();
        return apply_filters(system_error);
    }

    /** @brief Returns the state and counters learned from the kernel's error frames. */
    castle::status query_status(bus_status& out, int* system_error) CASTLE_NOEXCEPT
    {
        castle::io::fd::detail::clear_system_error(system_error);
        out = bus_status();
        if (!descriptor_.is_open())
        {
            return castle::status::not_configured;
        }
        out = status_;
        return castle::status::ok;
    }

private:
    /** @brief Applies close-on-exec and non-blocking mode through `fcntl()`. */
    static castle::status prepare_descriptor(castle::io::fd::basic_fd& descriptor, int* system_error) CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::status result = descriptor.set_close_on_exec(true, system_error);
        return castle::succeeded(result) ? descriptor.set_nonblocking(true, system_error) : result;
    }

    void start_session() CASTLE_NOEXCEPT
    {
        filters_.clear();
        status_ = bus_status();
        status_.state = bus_state::error_active;
    }

#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN

    static castle::status set_option(int descriptor,
                                     int option,
                                     CASTLE_CONST void* value,
                                     socklen_t length,
                                     int* system_error) CASTLE_NOEXCEPT
    {
        if (::setsockopt(descriptor, SOL_CAN_RAW, option, value, length) < 0)
        {
            castle::io::fd::detail::capture_system_error(system_error);
            return castle::status::system_call_error;
        }
        return castle::status::ok;
    }

    static castle::status bind_interface(int descriptor, CASTLE_CONST char* name, int* system_error) CASTLE_NOEXCEPT
    {
        struct sockaddr_can address = sockaddr_can();
        address.can_family = AF_CAN;
        address.can_ifindex = 0;

        if (strcmp(name, "any") != 0)
        {
            struct ifreq request = ifreq();
            strncpy(request.ifr_name, name, IFNAMSIZ - 1U);
            if (::ioctl(descriptor, SIOCGIFINDEX, &request) < 0)
            {
                castle::io::fd::detail::capture_system_error(system_error);
                return castle::status::system_call_error;
            }
            address.can_ifindex = request.ifr_ifindex;
        }
        if (::bind(descriptor, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0)
        {
            castle::io::fd::detail::capture_system_error(system_error);
            return castle::status::system_call_error;
        }
        return castle::status::ok;
    }

    /** @brief Pushes the occupied filter slots to the kernel; an empty table blocks all reception. */
    castle::status apply_filters(int* system_error) CASTLE_NOEXCEPT
    {
        struct can_filter active[MaxFilters];
        size_type count = 0U;
        for (size_type i = 0U; i < MaxFilters; ++i)
        {
            if (filters_.is_used(static_cast<filter_handle>(i)))
            {
                active[count++] = native::to_kernel_filter(filters_.at(static_cast<filter_handle>(i)));
            }
        }
        return set_option(descriptor_.native_handle(), CAN_RAW_FILTER,
                          (count != 0U) ? active : nullptr,
                          static_cast<socklen_t>(count * sizeof(struct can_filter)), system_error);
    }

    /** @brief Performs one non-blocking write; `ENOBUFS` (interface queue full) is reported as `full`. */
    castle::status write_frame(struct can_frame CASTLE_CONST& raw, int* system_error) CASTLE_NOEXCEPT
    {
        int error = 0;
        size_type written = 0U;
        castle::status result = castle::io::fd::write_fd(descriptor_.native_handle(), &raw, sizeof(raw), written, &error);
        if (result == castle::status::system_call_error && error == ENOBUFS)
        {
            result = castle::status::full;
        }
        else if (castle::succeeded(result) && written != sizeof(raw))
        {
            result = castle::status::data_loss;
        }
        if (system_error != nullptr)
        {
            *system_error = error;
        }
        return result;
    }

    static int to_poll_timeout(timeout_type timeout) CASTLE_NOEXCEPT
    {
        if (is_forever(timeout))
        {
            return -1;
        }
        return (timeout.count() > static_cast<int64_t>(INT_MAX)) ? INT_MAX : static_cast<int>(timeout.count());
    }

    /** @brief Milliseconds left until `deadline`, rounded up; zero once it has passed. */
    static int remaining_ms(castle::chrono::steady_clock::time_point deadline) CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::chrono::steady_clock::time_point now = castle::chrono::steady_clock::now();
        if (now >= deadline)
        {
            return 0;
        }
        CASTLE_CONST castle::chrono::milliseconds left =
            castle::chrono::ceil<castle::chrono::milliseconds>(deadline - now);
        return to_poll_timeout(left);
    }

#else

    castle::status apply_filters(int*) CASTLE_NOEXCEPT { return castle::status::not_configured; }

#endif // CASTLE_USING_IO_CAN_VIA_SOCKET_CAN

    castle::io::fd::basic_fd descriptor_;
    detail::slot_table<filter, MaxFilters> filters_;
    bus_status status_;
};

/** @brief SocketCAN backend with room for eight filters. */
using backend = basic_backend<>;

} // namespace socket_can
} // namespace can
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_CAN_BACKENDS_SOCKET_CAN_HPP
