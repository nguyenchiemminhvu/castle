#include <gtest/gtest.h>

#include "castle_ext/io/can/can.hpp"

#if CASTLE_USING_IO_CAN_VIA_SOCKET_CAN && !CASTLE_USING_IO_CAN_VIA_ZEPHYR
    #include <errno.h>
    #include <linux/can.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

namespace
{

using namespace castle::io::can;
using castle::status;

// ---------------------------------------------------------------- backend selection

TEST(CanDefaultBus, DefaultBackendFollowsBuildConfiguration)
{
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR
    EXPECT_TRUE((castle::meta::is_same<default_backend, zephyr::backend>::value));
    EXPECT_FALSE((castle::meta::is_same<default_backend, socket_can::backend>::value));
    EXPECT_FALSE((castle::meta::is_same<default_backend, mock::backend>::value));
#elif CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
    EXPECT_TRUE((castle::meta::is_same<default_backend, socket_can::backend>::value));
    EXPECT_FALSE((castle::meta::is_same<default_backend, mock::backend>::value));
#else
    EXPECT_TRUE((castle::meta::is_same<default_backend, mock::backend>::value));
#endif
}

TEST(CanDefaultBus, HardwareFlagIsConsistentWithMacros)
{
#if CASTLE_USING_IO_CAN_VIA_ZEPHYR || CASTLE_USING_IO_CAN_VIA_SOCKET_CAN
    EXPECT_TRUE(default_backend_is_hardware);
    EXPECT_FALSE((castle::meta::is_same<default_backend, mock::backend>::value));
#else
    EXPECT_FALSE(default_backend_is_hardware);
    EXPECT_TRUE((castle::meta::is_same<default_backend, mock::backend>::value));
#endif
    static_assert(default_backend_is_hardware == !castle::meta::is_same<default_backend, mock::backend>::value,
                  "hardware flag must mean exactly 'not the mock'");
}

TEST(CanDefaultBus, DefaultBusIsFacadeOverDefaultBackend)
{
    EXPECT_TRUE((castle::meta::is_same<default_bus, basic_bus<default_backend>>::value));
    EXPECT_TRUE((castle::meta::is_same<default_bus, bus<default_backend>>::value));
    EXPECT_TRUE((castle::meta::is_same<default_bus::backend_type, default_backend>::value));
    EXPECT_TRUE(detail::has_bus_backend_contract<default_backend>::value);
    EXPECT_EQ(sizeof(default_bus), sizeof(default_backend));
}

// ---------------------------------------------------------------- umbrella contents

TEST(CanDefaultBus, UmbrellaExposesValueTypesAtCompileTime)
{
    constexpr frame sample(0x123U);
    constexpr filter exact_filter = filter::exact(0x123U);
    constexpr filter any_extended = filter::any(id_format::extended);
    constexpr can_config config(bitrate_500k, can_mode::loopback);
    constexpr bus_status healthy;

    static_assert(sample.id == 0x123U && !sample.is_extended() && !sample.is_remote(), "frame");
    static_assert(exact_filter.matches(sample), "exact filter matches its own id");
    static_assert(!filter::exact(0x124U).matches(sample), "exact filter rejects other ids");
    static_assert(!any_extended.matches(sample), "format must match");
    static_assert(config.bitrate == 500000U && config.mode == can_mode::loopback, "config");
    static_assert(healthy.state == bus_state::stopped && healthy.rx_overruns == 0U, "status defaults");
    static_assert(is_operational(bus_state::error_active), "operational");
    static_assert(!is_operational(bus_state::bus_off), "bus-off is not operational");
    static_assert(is_forever(wait_forever) && !is_forever(no_wait), "timeouts");
    static_assert(max_id(id_format::standard) == 0x7FFU && max_id(id_format::extended) == 0x1FFFFFFFU, "ids");

    EXPECT_EQ(validate_frame(sample), status::ok);
    EXPECT_EQ(validate_filter(exact_filter), status::ok);
    EXPECT_EQ(validate_config(config), status::ok);
    EXPECT_EQ(invalid_filter, -1);
    EXPECT_EQ(max_data_length, 8U);
}

TEST(CanDefaultBus, UmbrellaPullsInMockBackend)
{
    basic_bus<mock::backend> bus;
    EXPECT_FALSE(bus.is_open());
    EXPECT_TRUE(detail::has_bus_backend_contract<mock::backend>::value);
}

// ---------------------------------------------------------------- round trip on this host

#if CASTLE_USING_IO_CAN_VIA_ZEPHYR

TEST(CanDefaultBus, ZephyrDefaultBusNeedsADeviceAndStaysClosedWithoutOne)
{
    default_bus bus;
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.open(nullptr), status::invalid_argument);
    EXPECT_FALSE(bus.is_open());
}

#elif CASTLE_USING_IO_CAN_VIA_SOCKET_CAN

bool kernel_can_available()
{
    const int probe = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (probe < 0)
    {
        return false;
    }
    ::close(probe);
    return true;
}

TEST(CanDefaultBus, SocketCanRoundTripOnAnyInterface)
{
    if (!kernel_can_available())
    {
        GTEST_SKIP() << "PF_CAN sockets are not available on this host";
    }

    default_bus bus;
    ASSERT_FALSE(bus.is_open());
    int error = 123;
    ASSERT_EQ(bus.open("any", can_config(bitrate_500k), &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_TRUE(bus.is_open());
    EXPECT_EQ(bus.open("any", can_config(bitrate_500k)), status::invalid_config);

    EXPECT_EQ(bus.accept_all(&error), status::ok);
    EXPECT_EQ(error, 0);

    // Nothing is queued on a fresh socket.
    frame received;
    EXPECT_EQ(bus.receive(received, no_wait, &error), status::empty);
    EXPECT_EQ(error, 0);
    error = 123;
    EXPECT_EQ(bus.receive(received, castle::chrono::milliseconds(5), &error), status::empty);
    EXPECT_EQ(error, 0);

    // A socket bound to "any" has no output interface. Either the host has a real, writable CAN
    // network (frame is queued) or the kernel refuses with an errno; both are reported faithfully.
    const uint8_t payload[2] = {0xCAU, 0xFEU};
    error = 123;
    const status sent = bus.send(0x123U, payload, 2U, id_format::standard, castle::chrono::milliseconds(10), &error);
    if (sent == status::ok)
    {
        EXPECT_EQ(error, 0);
    }
    else
    {
        EXPECT_EQ(sent, status::system_call_error);
        EXPECT_NE(error, 0);
    }

    bus_status st;
    error = 123;
    ASSERT_EQ(bus.query_status(st, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.clear_filters(), status::ok);

    ASSERT_EQ(bus.close(&error), status::ok);
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.send(0x1U, payload, 2U), status::not_configured);
    EXPECT_EQ(bus.close(), status::not_configured);
}

TEST(CanDefaultBus, SocketCanRejectsEmptyAndUnknownInterfaces)
{
    default_bus bus;
    EXPECT_EQ(bus.open(nullptr), status::invalid_argument);
    EXPECT_EQ(bus.open(""), status::invalid_argument);
    EXPECT_FALSE(bus.is_open());
}

#else

TEST(CanDefaultBus, MockDefaultBusLoopbackRoundTrip)
{
    default_bus bus;
    ASSERT_FALSE(bus.is_open());
    int error = 123;
    ASSERT_EQ(bus.open("can0", can_config(bitrate_500k, can_mode::loopback), &error), status::ok);
    EXPECT_EQ(error, 0);
    ASSERT_TRUE(bus.is_open());
    ASSERT_EQ(bus.accept_all(&error), status::ok);
    EXPECT_EQ(error, 0);

    frame received;
    EXPECT_EQ(bus.receive(received), status::empty);

    const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_EQ(bus.send(0x123U, payload, 8U), status::ok);
    ASSERT_EQ(bus.send(0x18DAF110U, payload, 3U, id_format::extended), status::ok);

    ASSERT_EQ(bus.receive(received, castle::chrono::milliseconds(10)), status::ok);
    EXPECT_EQ(received.id, 0x123U);
    EXPECT_FALSE(received.is_extended());
    EXPECT_EQ(received.dlc, 8U);
    EXPECT_EQ(received.data[7], 8U);

    ASSERT_EQ(bus.receive(received, castle::chrono::milliseconds(10)), status::ok);
    EXPECT_EQ(received.id, 0x18DAF110U);
    EXPECT_TRUE(received.is_extended());
    EXPECT_EQ(received.dlc, 3U);
    EXPECT_EQ(received.data[2], 3U);
    EXPECT_EQ(received.data[3], 0U);

    EXPECT_EQ(bus.receive(received), status::empty);

    bus_status st;
    ASSERT_EQ(bus.query_status(st), status::ok);
    EXPECT_EQ(st.state, bus_state::error_active);
    EXPECT_EQ(st.rx_overruns, 0U);

    ASSERT_EQ(bus.close(), status::ok);
    EXPECT_FALSE(bus.is_open());
}

TEST(CanDefaultBus, MockDefaultBusNormalModeQueuesForTheTestHarness)
{
    default_bus bus;
    ASSERT_EQ(bus.open("can0", can_config(bitrate_250k)), status::ok);
    ASSERT_EQ(bus.accept_all(), status::ok);

    const uint8_t payload[1] = {0x77U};
    ASSERT_EQ(bus.send(0x10U, payload, 1U), status::ok);
    frame received;
    EXPECT_EQ(bus.receive(received), status::empty);
    ASSERT_EQ(bus.backend().pop_sent(received), status::ok);
    EXPECT_EQ(received.id, 0x10U);
    EXPECT_EQ(received.data[0], 0x77U);

    ASSERT_EQ(bus.backend().inject(frame(0x20U)), status::ok);
    ASSERT_EQ(bus.receive(received), status::ok);
    EXPECT_EQ(received.id, 0x20U);
}

#endif

} // namespace
