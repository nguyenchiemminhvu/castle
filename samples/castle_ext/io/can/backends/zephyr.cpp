#include "sample_support.hpp"

#include "castle_ext/io/can/bus.hpp"
#include "castle_ext/io/can/backends/zephyr.hpp"

#include <errno.h>

using namespace castle::io::can;

int main()
{
#if !CASTLE_USING_IO_CAN_VIA_ZEPHYR
    // Host build: the Zephyr SDK is absent, so the backend is a stub with the full API surface.
    static_assert(!zephyr::zephyr_available, "no Zephyr CAN driver API in this build");

    // native::to_status() is available everywhere. It maps a driver return value
    // (0 or a negative errno) to a Castle status.
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(3) == castle::status::ok); // non-negative: success (e.g. a filter id)
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(-EAGAIN) == castle::status::full);
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(-ENOSPC) == castle::status::full);
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(-EINVAL) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(-ENOTSUP) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(-ERANGE) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(-ENETDOWN) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(-EIO) == castle::status::system_call_error);
    CASTLE_SAMPLE_CHECK(zephyr::native::to_status(-ENETUNREACH) == castle::status::system_call_error);

    // The endpoint is a `const struct device*`; a token address stands in for the devicetree handle.
    static const int fake_controller = 0;
    const struct device* controller = reinterpret_cast<const struct device*>(&fake_controller);

    bus<zephyr::backend> can;
    frame value(0x123U);
    frame rx;
    bus_status status;
    filter_handle handle = invalid_filter;

    // Every operation reports not_configured.
    CASTLE_SAMPLE_CHECK(can.open(controller, can_config(bitrate_500k)) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(!can.is_open());
    CASTLE_SAMPLE_CHECK(can.send(value) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.receive(rx, timeout_type(10)) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.add_filter(filter::any(), &handle) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(handle == invalid_filter);
    CASTLE_SAMPLE_CHECK(can.accept_all() == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.remove_filter(0) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.clear_filters() == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.close() == castle::status::not_configured);

    // The facade still validates before the stub sees anything, and a null device is rejected.
    CASTLE_SAMPLE_CHECK(can.open(controller, can_config(max_bitrate + 1U)) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(can.backend().open(nullptr, can_config(), nullptr) == castle::status::invalid_argument);

    // Queue and filter depths are template parameters, like on the real backend.
    zephyr::basic_backend<4U, 2U> compact;
    CASTLE_SAMPLE_CHECK(!compact.is_open());
#else
    // Zephyr build: the controller comes from the devicetree (e.g. an MCP2515 on SPI chosen as
    // `zephyr,canbus`); the same bus<> code as on every other platform follows.
    static_assert(zephyr::zephyr_available, "Zephyr CAN driver API present");

    const struct device* const controller = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    bus<zephyr::backend> can;

    CASTLE_SAMPLE_CHECK(can.open(controller, can_config(bitrate_500k)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.accept_all() == castle::status::ok);

    const uint8_t payload[2] = {0x01U, 0x02U};
    CASTLE_SAMPLE_CHECK(can.send(0x123U, payload, sizeof(payload), id_format::standard, timeout_type(100))
                        == castle::status::ok);

    // Frames reach receive() from the driver callback; without a peer nothing arrives in time.
    frame rx;
    const castle::status received = can.receive(rx, timeout_type(100));
    CASTLE_SAMPLE_CHECK(received == castle::status::ok || received == castle::status::empty);

    bus_status status;
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.close() == castle::status::ok);
#endif
    return 0;
}
