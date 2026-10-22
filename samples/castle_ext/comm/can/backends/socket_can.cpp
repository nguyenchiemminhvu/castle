#include "sample_support.hpp"

#include "castle_ext/comm/can/bus.hpp"
#include "castle_ext/comm/can/backends/socket_can.hpp"

#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
    #include <string.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

using namespace castle::comm::can;

int main()
{
#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
    static_assert(socket_can::socket_can_available, "SocketCAN UAPI is present in this build");

    // --- native:: conversions hold all knowledge of the kernel ABI --------------------------------

    // Standard data frame -> struct can_frame (no flags, zero-padded payload).
    frame standard(0x123U);
    CASTLE_SAMPLE_CHECK(standard.set_data({0x01U, 0x02U, 0x03U}) == castle::status::ok);
    struct can_frame raw;
    socket_can::native::to_kernel(standard, raw);
    CASTLE_SAMPLE_CHECK(raw.can_id == 0x123U);
    CASTLE_SAMPLE_CHECK(raw.can_dlc == 3U && raw.data[2] == 0x03U && raw.data[3] == 0U);

    // Extended identifiers carry CAN_EFF_FLAG, remote frames CAN_RTR_FLAG.
    frame extended(0x18FF0001U, id_format::extended);
    CASTLE_SAMPLE_CHECK(extended.set_remote(4U) == castle::status::ok);
    socket_can::native::to_kernel(extended, raw);
    CASTLE_SAMPLE_CHECK((raw.can_id & CAN_EFF_FLAG) != 0U);
    CASTLE_SAMPLE_CHECK((raw.can_id & CAN_RTR_FLAG) != 0U);
    CASTLE_SAMPLE_CHECK((raw.can_id & CAN_EFF_MASK) == 0x18FF0001U);
    CASTLE_SAMPLE_CHECK(raw.can_dlc == 4U);

    // Kernel -> Castle round trip keeps identifier, format, type and payload.
    frame decoded;
    socket_can::native::to_kernel(standard, raw);
    CASTLE_SAMPLE_CHECK(socket_can::native::from_kernel(raw, decoded) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decoded == standard);
    socket_can::native::to_kernel(extended, raw);
    CASTLE_SAMPLE_CHECK(socket_can::native::from_kernel(raw, decoded) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decoded == extended);

    // Error frames and DLCs the classic format cannot carry are data_loss.
    struct can_frame bad = can_frame();
    bad.can_id = CAN_ERR_FLAG | CAN_ERR_BUSOFF;
    CASTLE_SAMPLE_CHECK(socket_can::native::from_kernel(bad, decoded) == castle::status::data_loss);
    bad = can_frame();
    bad.can_id = 0x100U;
    bad.can_dlc = 15U;
    CASTLE_SAMPLE_CHECK(socket_can::native::from_kernel(bad, decoded) == castle::status::data_loss);

    // Filters: the extended flag is always part of the mask, so formats never mix.
    const struct can_filter exact_filter = socket_can::native::to_kernel_filter(filter::exact(0x123U));
    CASTLE_SAMPLE_CHECK(exact_filter.can_id == 0x123U);
    CASTLE_SAMPLE_CHECK(exact_filter.can_mask == (CAN_SFF_MASK | CAN_EFF_FLAG));
    const struct can_filter any_extended = socket_can::native::to_kernel_filter(filter::any(id_format::extended));
    CASTLE_SAMPLE_CHECK(any_extended.can_id == CAN_EFF_FLAG);
    CASTLE_SAMPLE_CHECK(any_extended.can_mask == CAN_EFF_FLAG);

    // Error frames the backend asks the kernel for.
    CASTLE_SAMPLE_CHECK((socket_can::native::error_mask & CAN_ERR_BUSOFF) != 0U);
    CASTLE_SAMPLE_CHECK((socket_can::native::error_mask & CAN_ERR_CRTL) != 0U);
    CASTLE_SAMPLE_CHECK((socket_can::native::error_mask & CAN_ERR_TX_TIMEOUT) != 0U);
    CASTLE_SAMPLE_CHECK((socket_can::native::error_mask & CAN_ERR_RESTARTED) != 0U);

    // apply_error_frame() folds error frames into a bus_status.
    bus_status folded;
    struct can_frame fault = can_frame();
    fault.can_id = CAN_ERR_FLAG | CAN_ERR_BUSOFF;
    socket_can::native::apply_error_frame(fault, folded);
    CASTLE_SAMPLE_CHECK(folded.state == bus_state::bus_off);
    fault = can_frame();
    fault.can_id = CAN_ERR_FLAG | CAN_ERR_RESTARTED | CAN_ERR_TX_TIMEOUT;
    socket_can::native::apply_error_frame(fault, folded);
    CASTLE_SAMPLE_CHECK(folded.state == bus_state::error_active && folded.tx_failures == 1U);
    fault = can_frame();
    fault.can_id = CAN_ERR_FLAG | CAN_ERR_CRTL;
    fault.data[1] = CAN_ERR_CRTL_RX_OVERFLOW | CAN_ERR_CRTL_RX_WARNING;
    socket_can::native::apply_error_frame(fault, folded);
    CASTLE_SAMPLE_CHECK(folded.state == bus_state::error_warning && folded.rx_overruns == 1U);

    // --- exchanging real struct can_frame datagrams over an AF_UNIX socketpair ---------------------
    // A unix datagram socket carries 16-byte frames just like CAN_RAW does for read/write/poll,
    // which makes it a stand-in for an interface when no vcan device exists.
    int ends[2] = {-1, -1};
    CASTLE_SAMPLE_CHECK(::socketpair(AF_UNIX, SOCK_DGRAM, 0, ends) == 0);

    bus<socket_can::backend> can;
    CASTLE_SAMPLE_CHECK(!can.is_open() && can.backend().native_handle() < 0);

    // Not open yet.
    frame rx;
    CASTLE_SAMPLE_CHECK(can.send(standard) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.receive(rx) == castle::status::not_configured);

    // adopt() takes over the descriptor (non-blocking, close-on-exec) and starts error_active.
    CASTLE_SAMPLE_CHECK(can.backend().adopt(-1) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(can.backend().adopt(ends[0]) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.is_open());
    CASTLE_SAMPLE_CHECK(can.backend().native_handle() == ends[0]);
    bus_status status;
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.state == bus_state::error_active);

    // Already open: open() and a second adopt() are refused. adopt() consumes its descriptor even then.
    CASTLE_SAMPLE_CHECK(can.open("can0") == castle::status::invalid_config);
    int spare[2] = {-1, -1};
    CASTLE_SAMPLE_CHECK(::socketpair(AF_UNIX, SOCK_DGRAM, 0, spare) == 0);
    CASTLE_SAMPLE_CHECK(can.backend().adopt(spare[0]) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(::close(spare[1]) == 0);

    // send(): the peer end reads the exact struct can_frame the backend wrote.
    CASTLE_SAMPLE_CHECK(can.send(standard) == castle::status::ok);
    struct can_frame on_wire;
    CASTLE_SAMPLE_CHECK(::read(ends[1], &on_wire, sizeof(on_wire)) == static_cast<ssize_t>(sizeof(on_wire)));
    CASTLE_SAMPLE_CHECK(on_wire.can_id == 0x123U && on_wire.can_dlc == 3U && on_wire.data[0] == 0x01U);

    const uint8_t payload[2] = {0xAAU, 0xBBU};
    CASTLE_SAMPLE_CHECK(can.send(0x18FF0001U, payload, sizeof(payload), id_format::extended) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(::read(ends[1], &on_wire, sizeof(on_wire)) == static_cast<ssize_t>(sizeof(on_wire)));
    CASTLE_SAMPLE_CHECK(on_wire.can_id == (0x18FF0001U | CAN_EFF_FLAG) && on_wire.can_dlc == 2U);

    // receive(): nothing pending is empty for no_wait and for a short timeout.
    CASTLE_SAMPLE_CHECK(can.receive(rx, no_wait) == castle::status::empty);
    CASTLE_SAMPLE_CHECK(can.receive(rx, timeout_type(20)) == castle::status::empty);

    // A datagram written by the peer arrives as a Castle frame.
    struct can_frame from_peer = can_frame();
    from_peer.can_id = 0x7E8U;
    from_peer.can_dlc = 2U;
    from_peer.data[0] = 0x41U;
    from_peer.data[1] = 0x0DU;
    CASTLE_SAMPLE_CHECK(::write(ends[1], &from_peer, sizeof(from_peer)) == static_cast<ssize_t>(sizeof(from_peer)));
    CASTLE_SAMPLE_CHECK(can.receive(rx, timeout_type(100)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(rx.id == 0x7E8U && rx.data_length() == 2U && rx.payload()[1] == 0x0DU);

    // Truncated datagram: a short read is reported as data_loss rather than a garbage frame.
    const uint8_t stub[4] = {1U, 2U, 3U, 4U};
    CASTLE_SAMPLE_CHECK(::write(ends[1], stub, sizeof(stub)) == static_cast<ssize_t>(sizeof(stub)));
    CASTLE_SAMPLE_CHECK(can.receive(rx, no_wait) == castle::status::data_loss);

    // Error frames are consumed inside receive() and only show up in query_status().
    struct can_frame error_frame = can_frame();
    error_frame.can_id = CAN_ERR_FLAG | CAN_ERR_CRTL;
    error_frame.data[1] = CAN_ERR_CRTL_TX_PASSIVE;
#ifdef CAN_ERR_CNT
    error_frame.can_id |= CAN_ERR_CNT;
#endif
    error_frame.data[6] = 140U;
    error_frame.data[7] = 7U;
    CASTLE_SAMPLE_CHECK(::write(ends[1], &error_frame, sizeof(error_frame)) == static_cast<ssize_t>(sizeof(error_frame)));
    CASTLE_SAMPLE_CHECK(can.receive(rx, no_wait) == castle::status::empty);
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.state == bus_state::error_passive && !is_operational(status.state));
    CASTLE_SAMPLE_CHECK(status.tx_error_counter == 140U && status.rx_error_counter == 7U);

    // Several error frames and a data frame in one go: the data frame still comes through.
    error_frame = can_frame();
    error_frame.can_id = CAN_ERR_FLAG | CAN_ERR_BUSOFF;
    CASTLE_SAMPLE_CHECK(::write(ends[1], &error_frame, sizeof(error_frame)) == static_cast<ssize_t>(sizeof(error_frame)));
    CASTLE_SAMPLE_CHECK(::write(ends[1], &from_peer, sizeof(from_peer)) == static_cast<ssize_t>(sizeof(from_peer)));
    CASTLE_SAMPLE_CHECK(can.receive(rx, timeout_type(100)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(rx.id == 0x7E8U);
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.state == bus_state::bus_off);
    error_frame = can_frame();
    error_frame.can_id = CAN_ERR_FLAG | CAN_ERR_RESTARTED;
    CASTLE_SAMPLE_CHECK(::write(ends[1], &error_frame, sizeof(error_frame)) == static_cast<ssize_t>(sizeof(error_frame)));
    CASTLE_SAMPLE_CHECK(can.receive(rx, no_wait) == castle::status::empty);
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.state == bus_state::error_active);

    // Filters go through setsockopt(SOL_CAN_RAW), which a unix socket lacks: the kernel's error is
    // reported and the slot is rolled back, so the table stays empty and handles stay invalid.
    int error = 0;
    filter_handle handle = 99;
    CASTLE_SAMPLE_CHECK(can.add_filter(filter::exact(0x123U), &handle, &error) == castle::status::system_call_error);
    CASTLE_SAMPLE_CHECK(error != 0 && handle == invalid_filter);
    CASTLE_SAMPLE_CHECK(can.accept_all(&error) == castle::status::system_call_error);
    CASTLE_SAMPLE_CHECK(can.remove_filter(0) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(can.clear_filters(&error) == castle::status::system_call_error);

    // close() forgets the session; a second close() reports not_configured.
    CASTLE_SAMPLE_CHECK(can.close() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!can.is_open() && can.backend().native_handle() < 0);
    CASTLE_SAMPLE_CHECK(can.close() == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(::close(ends[1]) == 0);

    // --- open() argument checking works without any CAN interface ----------------------------------
    socket_can::backend direct;
    int open_error = 0;
    CASTLE_SAMPLE_CHECK(direct.open(nullptr, can_config(), &open_error) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(direct.open("", can_config(), &open_error) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(direct.open("interface-name-too-long", can_config(), &open_error)
                        == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(direct.open("can0", can_config(bitrate_500k, can_mode::listen_only), &open_error)
                        == castle::status::invalid_config); // an interface property on Linux
    // Unknown interface (or no PF_CAN support at all): the errno explains why.
    CASTLE_SAMPLE_CHECK(direct.open("nonexistent0", can_config(), &open_error) == castle::status::system_call_error);
    CASTLE_SAMPLE_CHECK(open_error != 0);
    CASTLE_SAMPLE_CHECK(!direct.is_open());

    // The filter table size is a template parameter.
    socket_can::basic_backend<2U> small;
    CASTLE_SAMPLE_CHECK(!small.is_open());
#else
    // Without SocketCAN the backend keeps the API but every operation is not_configured.
    static_assert(!socket_can::socket_can_available, "stub build");

    socket_can::backend backend;
    bus<socket_can::backend> can;
    frame value(0x123U);
    frame rx;
    bus_status status;
    filter_handle handle = invalid_filter;

    CASTLE_SAMPLE_CHECK(can.open("can0") == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(backend.open("can0", can_config(), nullptr) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(!backend.is_open());
    CASTLE_SAMPLE_CHECK(can.send(value) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.receive(rx) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.add_filter(filter::any(), &handle) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(handle == invalid_filter);
    CASTLE_SAMPLE_CHECK(can.accept_all() == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.remove_filter(0) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.clear_filters() == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.close() == castle::status::not_configured);
#endif
    return 0;
}
