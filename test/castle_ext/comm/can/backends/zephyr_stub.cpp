// CASTLE_USING_IO_CAN_VIA_ZEPHYR is deliberately NOT defined here and no Zephyr headers are on the
// include path: this TU checks the "Zephyr unavailable" stub.
//
// ODR: zephyr.cpp instantiates zephyr::basic_backend<16,8>, <2,4>, <4,6>, <4,3> and <4,1> with the
// real implementation, so this TU must only instantiate other arguments (<3,2>), never
// zephyr::backend, to keep the two definitions from sharing a mangled symbol.
#include <gtest/gtest.h>

#include "castle_ext/comm/can/backends/zephyr.hpp"
#include "castle_ext/comm/can/bus.hpp"

#include <errno.h>
#include <stdint.h>

namespace
{

using namespace castle::comm::can;
using castle::status;

using stub_backend = zephyr::basic_backend<3U, 2U>;
using stub_bus = basic_bus<stub_backend>;

// The device type is incomplete here; any address will do because the stub never dereferences it.
const struct device* dummy_device()
{
    static int storage = 0;
    return reinterpret_cast<const struct device*>(&storage);
}

static_assert(detail::has_bus_backend_contract<stub_backend>::value, "stub must satisfy the bus backend contract");

TEST(CanZephyrStub, ZephyrIsNotAvailable)
{
    EXPECT_FALSE(zephyr::zephyr_available);
}

TEST(CanZephyrStub, ToStatusIsUnconditional)
{
    EXPECT_EQ(zephyr::native::to_status(0), status::ok);
    EXPECT_EQ(zephyr::native::to_status(-EAGAIN), status::full);
    EXPECT_EQ(zephyr::native::to_status(-ENOSPC), status::full);
    EXPECT_EQ(zephyr::native::to_status(-EINVAL), status::invalid_argument);
    EXPECT_EQ(zephyr::native::to_status(-ENOTSUP), status::invalid_config);
    EXPECT_EQ(zephyr::native::to_status(-ERANGE), status::invalid_config);
    EXPECT_EQ(zephyr::native::to_status(-ENETDOWN), status::not_configured);
    EXPECT_EQ(zephyr::native::to_status(-EIO), status::system_call_error);
}

TEST(CanZephyrStub, OpenWithNullDeviceIsInvalidArgument)
{
    stub_backend backend;
    int err = 9;
    EXPECT_EQ(backend.open(nullptr, can_config(bitrate_500k), &err), status::invalid_argument);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(backend.is_open());
}

TEST(CanZephyrStub, OpenWithDeviceIsNotConfiguredAndStaysClosed)
{
    stub_backend backend;
    int err = 9;
    EXPECT_EQ(backend.open(dummy_device(), can_config(bitrate_500k, can_mode::loopback), &err),
              status::not_configured);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(backend.open(dummy_device(), can_config(), nullptr), status::not_configured); // never "already open"
    EXPECT_FALSE(backend.is_open());
}

TEST(CanZephyrStub, EveryOperationIsNotConfigured)
{
    stub_backend backend;
    ASSERT_EQ(backend.open(dummy_device(), can_config(), nullptr), status::not_configured);

    int err = 5;
    EXPECT_EQ(backend.close(&err), status::not_configured);
    EXPECT_EQ(err, 0);

    err = 5;
    EXPECT_EQ(backend.send(frame(0x123U), timeout_type(10), &err), status::not_configured);
    EXPECT_EQ(err, 0);

    frame out(0x77U);
    err = 5;
    EXPECT_EQ(backend.receive(out, wait_forever, &err), status::not_configured);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(out.id, 0x77U);

    filter_handle handle = 3;
    err = 5;
    EXPECT_EQ(backend.add_filter(filter::any(), handle, &err), status::not_configured);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(handle, invalid_filter);

    err = 5;
    EXPECT_EQ(backend.remove_filter(0, &err), status::not_configured);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(backend.remove_filter(invalid_filter, nullptr), status::not_configured);

    err = 5;
    EXPECT_EQ(backend.clear_filters(&err), status::not_configured);
    EXPECT_EQ(err, 0);

    EXPECT_FALSE(backend.is_open());
}

TEST(CanZephyrStub, QueryStatusLeavesDefaultInitialisedOut)
{
    stub_backend backend;
    bus_status st;
    st.state = bus_state::bus_off;
    st.tx_error_counter = 7U;
    st.rx_error_counter = 8U;
    st.rx_overruns = 9U;
    st.tx_failures = 10U;

    int err = 4;
    EXPECT_EQ(backend.query_status(st, &err), status::not_configured);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(st.state, bus_state::stopped);
    EXPECT_EQ(st.tx_error_counter, 0U);
    EXPECT_EQ(st.rx_error_counter, 0U);
    EXPECT_EQ(st.rx_overruns, 0U);
    EXPECT_EQ(st.tx_failures, 0U);
}

TEST(CanZephyrStub, BusFacadeReportsNotConfiguredEverywhere)
{
    stub_bus bus;
    EXPECT_FALSE(bus.is_open());

    EXPECT_EQ(bus.open(nullptr), status::invalid_argument); // checked before availability
    EXPECT_EQ(bus.open(dummy_device(), can_config(bitrate_125k)), status::not_configured);
    EXPECT_EQ(bus.open(dummy_device(), can_config(max_bitrate + 1U)), status::invalid_config); // facade validation first
    EXPECT_FALSE(bus.is_open());

    EXPECT_EQ(bus.send(frame(0x1U)), status::not_configured);
    const uint8_t payload[] = {1U, 2U};
    EXPECT_EQ(bus.send(0x2U, payload, sizeof(payload)), status::not_configured);
    EXPECT_EQ(bus.send(frame(0x1U), timeout_type(-1)), status::invalid_argument);

    frame out;
    EXPECT_EQ(bus.receive(out), status::not_configured);
    EXPECT_EQ(bus.receive(out, timeout_type(-1)), status::invalid_argument);

    filter_handle handle = 7;
    EXPECT_EQ(bus.add_filter(filter::exact(0x10U), &handle), status::not_configured);
    EXPECT_EQ(handle, invalid_filter);
    EXPECT_EQ(bus.add_filter(filter::any()), status::not_configured);
    EXPECT_EQ(bus.accept_all(), status::not_configured);
    EXPECT_EQ(bus.remove_filter(0), status::not_configured);
    EXPECT_EQ(bus.clear_filters(), status::not_configured);

    bus_status st;
    st.rx_overruns = 3U;
    EXPECT_EQ(bus.query_status(st), status::not_configured);
    EXPECT_EQ(st.rx_overruns, 0U);
    EXPECT_EQ(st.state, bus_state::stopped);

    EXPECT_EQ(bus.close(), status::not_configured);
    EXPECT_FALSE(bus.backend().is_open());
}

TEST(CanZephyrStub, DestructorOfStubIsHarmless)
{
    {
        stub_backend backend;
        EXPECT_EQ(backend.open(dummy_device(), can_config(), nullptr), status::not_configured);
    }
    {
        stub_bus bus;
        EXPECT_EQ(bus.open(dummy_device()), status::not_configured);
    }
    SUCCEED();
}

} // namespace
