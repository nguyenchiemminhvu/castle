#include "sample_support.hpp"

#include "castle_ext/comm/can/can.hpp"

using namespace castle::comm::can;

// The only platform-specific line of application code is the endpoint passed to open().
static default_backend::endpoint_type default_endpoint()
{
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
    return DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
#elif CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
    return "any"; // every CAN interface, receive-only; use "can0" for a real interface
#else
    return "mock0";
#endif
}

// Generic over the bus type so the backend-specific branch is discarded at compile time.
template <typename Bus>
static void exercise(Bus& can)
{
    frame rx;

    int error = 0;
    const castle::status opened = can.open(default_endpoint(), can_config(bitrate_500k), &error);

    if constexpr (default_backend_is_hardware)
    {
        // A sandbox may lack PF_CAN support entirely: open() then reports system_call_error.
        CASTLE_SAMPLE_CHECK(opened == castle::status::ok || opened == castle::status::system_call_error);
        if (opened == castle::status::ok)
        {
            CASTLE_SAMPLE_CHECK(can.is_open());
            CASTLE_SAMPLE_CHECK(can.accept_all() == castle::status::ok);

            // Nothing waits on a fresh bind; no_wait never blocks. Live traffic would give ok instead.
            const castle::status polled = can.receive(rx, no_wait);
            CASTLE_SAMPLE_CHECK(polled == castle::status::empty || polled == castle::status::ok);

            bus_status status;
            CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::ok);
            CASTLE_SAMPLE_CHECK(status.state == bus_state::error_active);

            CASTLE_SAMPLE_CHECK(can.close() == castle::status::ok);
        }
        else
        {
            CASTLE_SAMPLE_CHECK(error != 0);
            CASTLE_SAMPLE_CHECK(!can.is_open());
        }
    }
    else
    {
        // Host fallback: the mock always opens and its wire is driven from the test side.
        CASTLE_SAMPLE_CHECK(opened == castle::status::ok);
        CASTLE_SAMPLE_CHECK(can.accept_all() == castle::status::ok);
        CASTLE_SAMPLE_CHECK(can.receive(rx, no_wait) == castle::status::empty);

        frame incoming(0x123U);
        CASTLE_SAMPLE_CHECK(incoming.set_data({0xBEU, 0xEFU}) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(can.backend().inject(incoming) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(can.receive(rx, no_wait) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(rx == incoming);
        CASTLE_SAMPLE_CHECK(can.close() == castle::status::ok);
    }

    CASTLE_SAMPLE_CHECK(!can.is_open());
}

int main()
{
    // The umbrella header pulls in every CAN type; default_bus follows the build configuration.
    static_assert(castle::meta::is_same<default_bus, basic_bus<default_backend>>::value, "");
    static_assert(castle::meta::is_same<default_bus::backend_type, default_backend>::value, "");
    static_assert(default_backend_is_hardware == (CASTLE_USING_IO_CAN_VIA_ZEPHYR || CASTLE_USING_IO_CAN_VIA_SOCKET_CAN),
                  "hardware default exactly when a hardware backend is configured");

    default_bus can;
    exercise(can);
    return 0;
}
