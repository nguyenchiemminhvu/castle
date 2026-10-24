#define CASTLE_USING_IO_CAN_VIA_ZEPHYR 1

#include <gtest/gtest.h>

#include "castle_ext/io/can/backends/zephyr.hpp"
#include "castle_ext/io/can/bus.hpp"

#include <errno.h>
#include <stdint.h>

// ODR: this TU instantiates zephyr::basic_backend<16,8> (the default backend), <2,4>, <4,6>, <4,3>
// and <4,1> with the real implementation. zephyr_stub.cpp only uses <3,2>.

namespace
{

using namespace castle::io::can;
using castle::status;

using default_zephyr_bus = basic_bus<zephyr::backend>;

// By value: `filter_slots` is a non-inline static member, so binding it to a reference would need a definition.
int fake_slots()
{
    const int slots = fake_can_controller::filter_slots;
    return slots;
}

void ignore_frame(const struct device*, struct can_frame*, void*)
{
}

struct can_frame native_frame(uint32_t id, uint8_t flags, uint8_t dlc, uint8_t first_byte = 0U)
{
    struct can_frame out = can_frame();
    out.id = id;
    out.flags = flags;
    out.dlc = dlc;
    for (uint8_t i = 0U; i < 8U; ++i)
    {
        out.data[i] = static_cast<uint8_t>(first_byte + i);
    }
    return out;
}

frame data_frame(uint32_t id, id_format format, uint8_t first_byte, castle::size_type length)
{
    frame out(id, format);
    uint8_t bytes[8] = {};
    for (uint8_t i = 0U; i < 8U; ++i)
    {
        bytes[i] = static_cast<uint8_t>(first_byte + i);
    }
    EXPECT_EQ(out.set_data(bytes, length), status::ok);
    return out;
}

class CanZephyrBackend : public ::testing::Test
{
protected:
    void SetUp() override
    {
        fake_can.reset();
        dev_.name = "can0";
        dev_.ready = true;
    }

    void TearDown() override { fake_can.reset(); }

    template <typename Backend>
    void open_ok(Backend& backend, can_config config = can_config())
    {
        int err = -1;
        ASSERT_EQ(backend.open(&dev_, config, &err), status::ok);
        ASSERT_EQ(err, 0);
        ASSERT_TRUE(backend.is_open());
    }

    struct device dev_ = {"can0", true};
};

// ---------------------------------------------------------------------------------------------
// native conversions
// ---------------------------------------------------------------------------------------------

TEST_F(CanZephyrBackend, ToStatusMapsEveryErrnoRow)
{
    EXPECT_TRUE(zephyr::zephyr_available);

    EXPECT_EQ(zephyr::native::to_status(0), status::ok);
    EXPECT_EQ(zephyr::native::to_status(3), status::ok);
    EXPECT_EQ(zephyr::native::to_status(-EAGAIN), status::full);
    EXPECT_EQ(zephyr::native::to_status(-ENOSPC), status::full);
    EXPECT_EQ(zephyr::native::to_status(-EINVAL), status::invalid_argument);
    EXPECT_EQ(zephyr::native::to_status(-ENOTSUP), status::invalid_config);
    EXPECT_EQ(zephyr::native::to_status(-ERANGE), status::invalid_config);
    EXPECT_EQ(zephyr::native::to_status(-ENETDOWN), status::not_configured);
    EXPECT_EQ(zephyr::native::to_status(-EIO), status::system_call_error);
    EXPECT_EQ(zephyr::native::to_status(-EBUSY), status::system_call_error);
    EXPECT_EQ(zephyr::native::to_status(-ENETUNREACH), status::system_call_error);
}

TEST_F(CanZephyrBackend, ToZephyrConvertsStandardDataFrame)
{
    const frame in = data_frame(0x123U, id_format::standard, 0x10U, 3U);
    struct can_frame out = native_frame(0xFFFFFFFFU, 0xFFU, 0xFFU, 0xAAU);
    out.reserved = 0xBEEFU;

    zephyr::native::to_zephyr(in, out);

    EXPECT_EQ(out.id, 0x123U);
    EXPECT_EQ(out.dlc, 3U);
    EXPECT_EQ(out.flags, 0U);
    EXPECT_EQ(out.reserved, 0U);
    EXPECT_EQ(out.data[0], 0x10U);
    EXPECT_EQ(out.data[1], 0x11U);
    EXPECT_EQ(out.data[2], 0x12U);
    for (int i = 3; i < 8; ++i)
    {
        EXPECT_EQ(out.data[i], 0U) << "byte " << i;
    }
}

TEST_F(CanZephyrBackend, ToZephyrCopiesOnlyDataLengthBytes)
{
    frame in = data_frame(0x1U, id_format::standard, 0x20U, 8U);
    in.dlc = 2U; // bytes 2..7 are stale and must not leak
    struct can_frame out = native_frame(0U, 0U, 0U, 0xEEU);

    zephyr::native::to_zephyr(in, out);

    EXPECT_EQ(out.dlc, 2U);
    EXPECT_EQ(out.data[0], 0x20U);
    EXPECT_EQ(out.data[1], 0x21U);
    for (int i = 2; i < 8; ++i)
    {
        EXPECT_EQ(out.data[i], 0U) << "byte " << i;
    }
}

TEST_F(CanZephyrBackend, ToZephyrSetsExtendedAndRemoteFlags)
{
    const frame extended = data_frame(0x18FF0001U, id_format::extended, 0x30U, 8U);
    struct can_frame out = can_frame();
    zephyr::native::to_zephyr(extended, out);
    EXPECT_EQ(out.id, 0x18FF0001U);
    EXPECT_EQ(out.flags, CAN_FRAME_IDE);
    EXPECT_EQ(out.dlc, 8U);
    EXPECT_EQ(out.data[7], 0x37U);

    frame remote(0x7FFU);
    ASSERT_EQ(remote.set_remote(4U), status::ok);
    zephyr::native::to_zephyr(remote, out);
    EXPECT_EQ(out.id, 0x7FFU);
    EXPECT_EQ(out.flags, CAN_FRAME_RTR);
    EXPECT_EQ(out.dlc, 4U);
    for (int i = 0; i < 8; ++i)
    {
        EXPECT_EQ(out.data[i], 0U) << "byte " << i;
    }

    frame extended_remote(0x1FFFFFFFU, id_format::extended);
    ASSERT_EQ(extended_remote.set_remote(0U), status::ok);
    zephyr::native::to_zephyr(extended_remote, out);
    EXPECT_EQ(out.id, 0x1FFFFFFFU);
    EXPECT_EQ(out.flags, CAN_FRAME_IDE | CAN_FRAME_RTR);
    EXPECT_EQ(out.dlc, 0U);
}

TEST_F(CanZephyrBackend, FromZephyrConvertsStandardAndExtendedData)
{
    frame out;
    ASSERT_EQ(zephyr::native::from_zephyr(native_frame(0x123U, 0U, 3U, 0x40U), out), status::ok);
    EXPECT_EQ(out.id, 0x123U);
    EXPECT_EQ(out.format, id_format::standard);
    EXPECT_EQ(out.type, frame_type::data);
    EXPECT_EQ(out.dlc, 3U);
    EXPECT_EQ(out.data[0], 0x40U);
    EXPECT_EQ(out.data[2], 0x42U);
    EXPECT_EQ(out.data[3], 0U); // beyond dlc zeroed by set_data

    ASSERT_EQ(zephyr::native::from_zephyr(native_frame(0x18FF0001U, CAN_FRAME_IDE, 8U, 1U), out), status::ok);
    EXPECT_EQ(out.id, 0x18FF0001U);
    EXPECT_EQ(out.format, id_format::extended);
    EXPECT_EQ(out.type, frame_type::data);
    EXPECT_EQ(out.dlc, 8U);
    EXPECT_EQ(out.data[7], 8U);

    ASSERT_EQ(zephyr::native::from_zephyr(native_frame(0x5U, 0U, 0U, 9U), out), status::ok);
    EXPECT_EQ(out.dlc, 0U);
    EXPECT_EQ(out.data[0], 0U);
}

TEST_F(CanZephyrBackend, FromZephyrConvertsRemoteFrames)
{
    frame out;
    ASSERT_EQ(zephyr::native::from_zephyr(native_frame(0x321U, CAN_FRAME_RTR, 5U, 0x77U), out), status::ok);
    EXPECT_TRUE(out.is_remote());
    EXPECT_FALSE(out.is_extended());
    EXPECT_EQ(out.id, 0x321U);
    EXPECT_EQ(out.dlc, 5U);
    EXPECT_EQ(out.data_length(), 0U);
    for (castle::size_type i = 0U; i < max_data_length; ++i)
    {
        EXPECT_EQ(out.data[i], 0U) << "byte " << i;
    }

    ASSERT_EQ(zephyr::native::from_zephyr(native_frame(0x1ABCDEFU, CAN_FRAME_RTR | CAN_FRAME_IDE, 2U), out),
              status::ok);
    EXPECT_TRUE(out.is_remote());
    EXPECT_TRUE(out.is_extended());
    EXPECT_EQ(out.dlc, 2U);
}

TEST_F(CanZephyrBackend, FromZephyrRejectsFdAndLongDlcWithDataLoss)
{
    frame out = data_frame(0x55U, id_format::standard, 1U, 2U);
    const frame before = out;

    EXPECT_EQ(zephyr::native::from_zephyr(native_frame(0x1U, CAN_FRAME_FDF, 4U), out), status::data_loss);
    EXPECT_EQ(out, before);
    EXPECT_EQ(zephyr::native::from_zephyr(native_frame(0x1U, CAN_FRAME_FDF | CAN_FRAME_IDE, 0U), out),
              status::data_loss);
    EXPECT_EQ(zephyr::native::from_zephyr(native_frame(0x1U, 0U, 9U), out), status::data_loss);
    EXPECT_EQ(zephyr::native::from_zephyr(native_frame(0x1U, CAN_FRAME_RTR, 15U), out), status::data_loss);
    EXPECT_EQ(out, before);

    EXPECT_EQ(zephyr::native::from_zephyr(native_frame(0x1U, 0U, 8U), out), status::ok);
}

TEST_F(CanZephyrBackend, ToZephyrFilterForwardsIdMaskAndFormat)
{
    struct can_filter standard = zephyr::native::to_zephyr_filter(filter(0x120U, 0x7F0U));
    EXPECT_EQ(standard.id, 0x120U);
    EXPECT_EQ(standard.mask, 0x7F0U);
    EXPECT_EQ(standard.flags, 0U);

    struct can_filter extended = zephyr::native::to_zephyr_filter(filter::exact(0x18FF0001U, id_format::extended));
    EXPECT_EQ(extended.id, 0x18FF0001U);
    EXPECT_EQ(extended.mask, max_extended_id);
    EXPECT_EQ(extended.flags, CAN_FILTER_IDE);

    struct can_filter any = zephyr::native::to_zephyr_filter(filter::any(id_format::standard));
    EXPECT_EQ(any.id, 0U);
    EXPECT_EQ(any.mask, 0U);
    EXPECT_EQ(any.flags, 0U);
}

TEST_F(CanZephyrBackend, ToZephyrModeMapsEveryMode)
{
    EXPECT_EQ(zephyr::native::to_zephyr_mode(can_mode::normal), CAN_MODE_NORMAL);
    EXPECT_EQ(zephyr::native::to_zephyr_mode(can_mode::loopback), CAN_MODE_LOOPBACK);
    EXPECT_EQ(zephyr::native::to_zephyr_mode(can_mode::listen_only), CAN_MODE_LISTENONLY);
    EXPECT_EQ(zephyr::native::to_zephyr_mode(static_cast<can_mode>(77U)), CAN_MODE_NORMAL);
}

TEST_F(CanZephyrBackend, ToBusStateMapsEveryState)
{
    EXPECT_EQ(zephyr::native::to_bus_state(CAN_STATE_ERROR_ACTIVE), bus_state::error_active);
    EXPECT_EQ(zephyr::native::to_bus_state(CAN_STATE_ERROR_WARNING), bus_state::error_warning);
    EXPECT_EQ(zephyr::native::to_bus_state(CAN_STATE_ERROR_PASSIVE), bus_state::error_passive);
    EXPECT_EQ(zephyr::native::to_bus_state(CAN_STATE_BUS_OFF), bus_state::bus_off);
    EXPECT_EQ(zephyr::native::to_bus_state(CAN_STATE_STOPPED), bus_state::stopped);
    EXPECT_EQ(zephyr::native::to_bus_state(static_cast<enum can_state>(99)), bus_state::stopped);
}

TEST_F(CanZephyrBackend, ToKernelTimeoutHandlesForeverZeroNegativeAndClamp)
{
    EXPECT_EQ(zephyr::native::to_kernel_timeout(wait_forever).ticks, K_FOREVER.ticks);
    EXPECT_EQ(zephyr::native::to_kernel_timeout(no_wait).ticks, K_NO_WAIT.ticks);
    EXPECT_EQ(zephyr::native::to_kernel_timeout(timeout_type(-5)).ticks, K_NO_WAIT.ticks);
    EXPECT_EQ(zephyr::native::to_kernel_timeout(timeout_type(1)).ticks, 1);
    EXPECT_EQ(zephyr::native::to_kernel_timeout(timeout_type(250)).ticks, 250);
    EXPECT_EQ(zephyr::native::to_kernel_timeout(timeout_type(INT32_MAX)).ticks, INT32_MAX);
    EXPECT_EQ(zephyr::native::to_kernel_timeout(timeout_type(static_cast<int64_t>(INT32_MAX) + 1)).ticks, INT32_MAX);
    EXPECT_EQ(zephyr::native::to_kernel_timeout(timeout_type::max() - timeout_type(1)).ticks, INT32_MAX);
}

// ---------------------------------------------------------------------------------------------
// open / close
// ---------------------------------------------------------------------------------------------

TEST_F(CanZephyrBackend, StartsClosed)
{
    zephyr::backend backend;
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(fake_can.stop_calls, 0);
    EXPECT_EQ(fake_can.start_calls, 0);
}

TEST_F(CanZephyrBackend, OpenAppliesBitrateModeAndStartsController)
{
    fake_can.mode = 0xFFU;
    zephyr::backend backend;
    int err = 55;
    ASSERT_EQ(backend.open(&dev_, can_config(bitrate_500k, can_mode::loopback), &err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_TRUE(backend.is_open());
    EXPECT_EQ(fake_can.bitrate, bitrate_500k);
    EXPECT_EQ(fake_can.mode, CAN_MODE_LOOPBACK);
    EXPECT_TRUE(fake_can.started);
    EXPECT_EQ(fake_can.start_calls, 1);
    EXPECT_EQ(fake_can.installed_filters(), 0);
}

TEST_F(CanZephyrBackend, OpenForwardsEveryMode)
{
    {
        fake_can.mode = 0xFFU;
        zephyr::backend backend;
        ASSERT_EQ(backend.open(&dev_, can_config(0U, can_mode::normal), nullptr), status::ok);
        EXPECT_EQ(fake_can.mode, CAN_MODE_NORMAL);
    }
    {
        zephyr::backend backend;
        ASSERT_EQ(backend.open(&dev_, can_config(0U, can_mode::listen_only), nullptr), status::ok);
        EXPECT_EQ(fake_can.mode, CAN_MODE_LISTENONLY);
    }
}

TEST_F(CanZephyrBackend, ZeroBitrateKeepsDeviceTreeDefault)
{
    fake_can.bitrate = 123456U; // sentinel: set_bitrate would overwrite it
    zephyr::backend backend;
    ASSERT_EQ(backend.open(&dev_, can_config(0U), nullptr), status::ok);
    EXPECT_EQ(fake_can.bitrate, 123456U);
    EXPECT_TRUE(fake_can.started);
}

TEST_F(CanZephyrBackend, OpenRejectsNullDeviceWithInvalidArgument)
{
    zephyr::backend backend;
    int err = 9;
    EXPECT_EQ(backend.open(nullptr, can_config(), &err), status::invalid_argument);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(fake_can.stop_calls, 0);

    ASSERT_EQ(backend.open(&dev_, can_config(), nullptr), status::ok);
    err = 9;
    EXPECT_EQ(backend.open(nullptr, can_config(), &err), status::invalid_argument); // null wins over "already open"
    EXPECT_EQ(err, 0);
}

TEST_F(CanZephyrBackend, DoubleOpenIsInvalidConfigAndTouchesNothing)
{
    zephyr::backend backend;
    ASSERT_EQ(backend.open(&dev_, can_config(bitrate_250k), nullptr), status::ok);
    const int stops = fake_can.stop_calls;
    const int starts = fake_can.start_calls;

    int err = 4;
    EXPECT_EQ(backend.open(&dev_, can_config(bitrate_500k), &err), status::invalid_config);
    EXPECT_EQ(err, 0);
    EXPECT_TRUE(backend.is_open());
    EXPECT_EQ(fake_can.stop_calls, stops);
    EXPECT_EQ(fake_can.start_calls, starts);
    EXPECT_EQ(fake_can.bitrate, bitrate_250k);
}

TEST_F(CanZephyrBackend, OpenFailsWhenDeviceNotReady)
{
    dev_.ready = false;
    zephyr::backend backend;
    EXPECT_EQ(backend.open(&dev_, can_config(bitrate_500k), nullptr), status::not_configured);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(fake_can.stop_calls, 0);
    EXPECT_EQ(fake_can.start_calls, 0);
    EXPECT_EQ(fake_can.bitrate, 0U);

    dev_.ready = true;
    EXPECT_EQ(backend.open(&dev_, can_config(bitrate_500k), nullptr), status::ok);
}

TEST_F(CanZephyrBackend, OpenToleratesAlreadyStoppedController)
{
    ASSERT_FALSE(fake_can.started); // fake can_stop() reports -EALREADY
    zephyr::backend backend;
    int err = -1;
    EXPECT_EQ(backend.open(&dev_, can_config(bitrate_125k), &err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(fake_can.stop_calls, 1);
    EXPECT_EQ(fake_can.bitrate, bitrate_125k);
    EXPECT_TRUE(fake_can.started);
}

TEST_F(CanZephyrBackend, OpenStopsRunningControllerBeforeConfiguring)
{
    fake_can.started = true;
    zephyr::backend backend;
    ASSERT_EQ(backend.open(&dev_, can_config(bitrate_1m), nullptr), status::ok);
    EXPECT_EQ(fake_can.stop_calls, 1);
    EXPECT_EQ(fake_can.start_calls, 1);
    EXPECT_EQ(fake_can.bitrate, bitrate_1m);
    EXPECT_TRUE(fake_can.started);
}

TEST_F(CanZephyrBackend, OpenFailuresAreMappedAndLeaveDeviceClosed)
{
    struct scenario
    {
        int* script;
        int code;
        status expected;
        int expected_errno;
    };
    const scenario scenarios[] = {
        {&fake_can.stop_result, -EIO, status::system_call_error, EIO},
        {&fake_can.set_mode_result, -ENOTSUP, status::invalid_config, ENOTSUP},
        {&fake_can.set_bitrate_result, -ERANGE, status::invalid_config, ERANGE},
        {&fake_can.start_result, -ENETDOWN, status::not_configured, ENETDOWN},
        {&fake_can.start_result, -EINVAL, status::invalid_argument, EINVAL},
    };

    for (const scenario& s : scenarios)
    {
        fake_can.reset();
        *s.script = s.code;
        zephyr::backend backend;
        int err = 0;
        EXPECT_EQ(backend.open(&dev_, can_config(bitrate_500k), &err), s.expected) << "code " << s.code;
        EXPECT_EQ(err, s.expected_errno) << "code " << s.code;
        EXPECT_FALSE(backend.is_open());

        frame f;
        EXPECT_EQ(backend.receive(f, no_wait, nullptr), status::not_configured);
        EXPECT_EQ(backend.send(f, no_wait, nullptr), status::not_configured);
        EXPECT_EQ(fake_can.send_calls, 0);

        *s.script = 0;
        EXPECT_EQ(backend.open(&dev_, can_config(bitrate_500k), nullptr), status::ok) << "retry after " << s.code;
    }
}

TEST_F(CanZephyrBackend, OpenStopFailureDoesNotTouchModeOrBitrate)
{
    fake_can.stop_result = -EIO;
    fake_can.bitrate = 777U;
    fake_can.mode = 0xFFU;
    zephyr::backend backend;
    EXPECT_EQ(backend.open(&dev_, can_config(bitrate_500k, can_mode::loopback), nullptr), status::system_call_error);
    EXPECT_EQ(fake_can.bitrate, 777U);
    EXPECT_EQ(fake_can.mode, 0xFFU);
    EXPECT_EQ(fake_can.start_calls, 0);
}

TEST_F(CanZephyrBackend, CloseRemovesFiltersStopsControllerAndReleasesDevice)
{
    zephyr::backend backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::exact(0x10U), h, nullptr), status::ok);
    ASSERT_EQ(backend.add_filter(filter::exact(0x20U, id_format::extended), h, nullptr), status::ok);
    ASSERT_EQ(fake_can.installed_filters(), 2);
    const int stops = fake_can.stop_calls;

    int err = 3;
    EXPECT_EQ(backend.close(&err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(fake_can.remove_calls, 2);
    EXPECT_EQ(fake_can.installed_filters(), 0);
    EXPECT_EQ(fake_can.stop_calls, stops + 1);
    EXPECT_FALSE(fake_can.started);
}

TEST_F(CanZephyrBackend, CloseTwiceIsNotConfigured)
{
    zephyr::backend backend;
    int err = 8;
    EXPECT_EQ(backend.close(&err), status::not_configured); // never opened
    EXPECT_EQ(err, 0);
    EXPECT_EQ(fake_can.stop_calls, 0);

    open_ok(backend);
    ASSERT_EQ(backend.close(nullptr), status::ok);
    const int stops = fake_can.stop_calls;
    EXPECT_EQ(backend.close(nullptr), status::not_configured);
    EXPECT_EQ(fake_can.stop_calls, stops);
}

TEST_F(CanZephyrBackend, CloseReportsStopFailureButStillReleases)
{
    zephyr::backend backend;
    open_ok(backend);
    fake_can.stop_result = -EIO;
    int err = 0;
    EXPECT_EQ(backend.close(&err), status::system_call_error);
    EXPECT_EQ(err, EIO);
    EXPECT_FALSE(backend.is_open());

    fake_can.stop_result = 0;
    EXPECT_EQ(backend.close(nullptr), status::not_configured);
}

TEST_F(CanZephyrBackend, CloseToleratesAlreadyStoppedController)
{
    zephyr::backend backend;
    open_ok(backend);
    fake_can.started = false; // controller stopped behind our back -> can_stop() == -EALREADY
    int err = 5;
    EXPECT_EQ(backend.close(&err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(backend.is_open());
}

TEST_F(CanZephyrBackend, DestructorOfOpenBackendUnregistersFiltersAndStopsController)
{
    {
        default_zephyr_bus bus;
        ASSERT_EQ(bus.open(&dev_, can_config(bitrate_500k)), status::ok);
        ASSERT_EQ(bus.accept_all(), status::ok);
        ASSERT_EQ(fake_can.installed_filters(), 2);
        ASSERT_TRUE(fake_can.started);
    }
    EXPECT_EQ(fake_can.installed_filters(), 0);
    EXPECT_EQ(fake_can.remove_calls, 2);
    EXPECT_FALSE(fake_can.started);
}

TEST_F(CanZephyrBackend, DestructorOfClosedBackendDoesNotTouchController)
{
    {
        zephyr::backend backend;
    }
    EXPECT_EQ(fake_can.stop_calls, 0);
    EXPECT_EQ(fake_can.remove_calls, 0);
}

// ---------------------------------------------------------------------------------------------
// send
// ---------------------------------------------------------------------------------------------

TEST_F(CanZephyrBackend, SendConvertsFrameAndTimeoutAndRegistersTxCallback)
{
    zephyr::backend backend;
    open_ok(backend);

    const frame f = data_frame(0x18FF0001U, id_format::extended, 0xA0U, 5U);
    int err = 6;
    ASSERT_EQ(backend.send(f, timeout_type(250), &err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(fake_can.send_calls, 1);
    EXPECT_EQ(fake_can.sent_frame.id, 0x18FF0001U);
    EXPECT_EQ(fake_can.sent_frame.flags, CAN_FRAME_IDE);
    EXPECT_EQ(fake_can.sent_frame.dlc, 5U);
    EXPECT_EQ(fake_can.sent_frame.data[0], 0xA0U);
    EXPECT_EQ(fake_can.sent_frame.data[4], 0xA4U);
    EXPECT_EQ(fake_can.sent_frame.data[5], 0U);
    EXPECT_EQ(fake_can.send_timeout.ticks, 250);
    EXPECT_NE(fake_can.tx_callback, nullptr);
    EXPECT_EQ(fake_can.tx_user_data, static_cast<void*>(&backend));

    frame remote(0x42U);
    ASSERT_EQ(remote.set_remote(3U), status::ok);
    ASSERT_EQ(backend.send(remote, no_wait, nullptr), status::ok);
    EXPECT_EQ(fake_can.sent_frame.flags, CAN_FRAME_RTR);
    EXPECT_EQ(fake_can.sent_frame.dlc, 3U);
    EXPECT_EQ(fake_can.send_timeout.ticks, K_NO_WAIT.ticks);

    ASSERT_EQ(backend.send(f, wait_forever, nullptr), status::ok);
    EXPECT_EQ(fake_can.send_timeout.ticks, K_FOREVER.ticks);
    EXPECT_EQ(fake_can.send_calls, 3);
}

TEST_F(CanZephyrBackend, SendErrorsAreMappedWithErrno)
{
    zephyr::backend backend;
    open_ok(backend);
    const frame f = data_frame(0x1U, id_format::standard, 0U, 1U);

    int err = 0;
    fake_can.send_result = -EAGAIN;
    EXPECT_EQ(backend.send(f, timeout_type(5), &err), status::full);
    EXPECT_EQ(err, EAGAIN);

    err = 0;
    fake_can.send_result = -ENETDOWN;
    EXPECT_EQ(backend.send(f, no_wait, &err), status::not_configured);
    EXPECT_EQ(err, ENETDOWN);

    err = 0;
    fake_can.send_result = -ENETUNREACH;
    EXPECT_EQ(backend.send(f, no_wait, &err), status::system_call_error);
    EXPECT_EQ(err, ENETUNREACH);

    err = 0;
    fake_can.send_result = -EINVAL;
    EXPECT_EQ(backend.send(f, no_wait, &err), status::invalid_argument);
    EXPECT_EQ(err, EINVAL);

    fake_can.send_result = -EAGAIN;
    EXPECT_EQ(backend.send(f, no_wait, nullptr), status::full); // null out-param is fine

    fake_can.send_result = 0;
    err = 12;
    EXPECT_EQ(backend.send(f, no_wait, &err), status::ok);
    EXPECT_EQ(err, 0);
}

TEST_F(CanZephyrBackend, SendOnClosedBackendIsNotConfigured)
{
    zephyr::backend backend;
    int err = 7;
    EXPECT_EQ(backend.send(frame(0x1U), no_wait, &err), status::not_configured);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(fake_can.send_calls, 0);
}

TEST_F(CanZephyrBackend, TxCompletionErrorsAreCountedAsFailures)
{
    zephyr::backend backend;
    open_ok(backend);
    const frame f = data_frame(0x2U, id_format::standard, 0U, 2U);

    ASSERT_EQ(backend.send(f, no_wait, nullptr), status::ok);
    ASSERT_NE(fake_can.tx_callback, nullptr);

    bus_status st;
    fake_can.tx_callback(&dev_, 0, fake_can.tx_user_data);
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.tx_failures, 0U);

    fake_can.tx_callback(&dev_, -EIO, fake_can.tx_user_data);
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.tx_failures, 1U);

    fake_can.tx_callback(&dev_, 0, fake_can.tx_user_data);
    fake_can.tx_callback(&dev_, -ENETDOWN, fake_can.tx_user_data);
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.tx_failures, 2U);
    EXPECT_EQ(st.rx_overruns, 0U);
}

// ---------------------------------------------------------------------------------------------
// receive
// ---------------------------------------------------------------------------------------------

TEST_F(CanZephyrBackend, ReceiveOnClosedBackendIsNotConfigured)
{
    zephyr::backend backend;
    frame out = frame(0x77U);
    int err = 1;
    EXPECT_EQ(backend.receive(out, no_wait, &err), status::not_configured);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(out.id, 0x77U);
}

TEST_F(CanZephyrBackend, ReceiveYieldsNothingBeforeFiltersAreInstalled)
{
    zephyr::backend backend;
    open_ok(backend);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x1U, 0U, 1U)), 0);
    frame out = frame(0x77U);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
    EXPECT_EQ(out.id, 0x77U);
}

TEST_F(CanZephyrBackend, ReceiveReturnsFramesInFifoOrder)
{
    zephyr::backend backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(id_format::standard), h, nullptr), status::ok);
    ASSERT_EQ(backend.add_filter(filter::any(id_format::extended), h, nullptr), status::ok);

    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x100U, 0U, 2U, 0x10U)), 1);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x1ABCDEFU, CAN_FRAME_IDE, 8U, 0x20U)), 1);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x300U, CAN_FRAME_RTR, 4U)), 1);

    frame out;
    int err = 5;
    ASSERT_EQ(backend.receive(out, no_wait, &err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(out, data_frame(0x100U, id_format::standard, 0x10U, 2U));

    ASSERT_EQ(backend.receive(out, timeout_type(10), nullptr), status::ok);
    EXPECT_EQ(out, data_frame(0x1ABCDEFU, id_format::extended, 0x20U, 8U));

    ASSERT_EQ(backend.receive(out, wait_forever, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x300U);
    EXPECT_TRUE(out.is_remote());
    EXPECT_EQ(out.dlc, 4U);

    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
}

TEST_F(CanZephyrBackend, ReceiveReturnsEmptyOnNoWaitAndOnTimeout)
{
    zephyr::backend backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), h, nullptr), status::ok);

    frame out = frame(0x77U);
    int err = 3;
    EXPECT_EQ(backend.receive(out, no_wait, &err), status::empty); // -EBUSY from k_sem_take
    EXPECT_EQ(err, 0);
    EXPECT_EQ(backend.receive(out, timeout_type(20), &err), status::empty); // -EAGAIN from k_sem_take
    EXPECT_EQ(err, 0);
    EXPECT_EQ(backend.receive(out, wait_forever, nullptr), status::empty);
    EXPECT_EQ(out.id, 0x77U);
}

TEST_F(CanZephyrBackend, FdFramesAndOversizedDlcAreDroppedSilently)
{
    zephyr::backend backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), h, nullptr), status::ok);

    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x1U, CAN_FRAME_FDF, 8U)), 1);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x2U, 0U, 12U)), 1);

    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.rx_overruns, 0U); // dropped by conversion, not by queue pressure

    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x3U, 0U, 1U, 0x5AU)), 1);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x3U);
    EXPECT_EQ(out.data[0], 0x5AU);
}

TEST_F(CanZephyrBackend, ReceiveQueueOverflowDropsNewFramesAndCountsOverruns)
{
    zephyr::basic_backend<2U, 4U> backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), h, nullptr), status::ok);

    for (uint32_t id = 1U; id <= 5U; ++id)
    {
        EXPECT_EQ(fake_can.deliver(&dev_, native_frame(id, 0U, 0U)), 1);
    }

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.rx_overruns, 3U);

    frame out;
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 1U); // oldest frames kept, newest dropped
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 2U);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
    EXPECT_EQ(backend.receive(out, timeout_type(5), nullptr), status::empty);

    // The queue and the semaphore stay in step after the overflow: exactly the new frames come out.
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x10U, 0U, 1U, 0x33U)), 1);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x11U, 0U, 1U, 0x44U)), 1);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x10U);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x11U);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);

    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.rx_overruns, 3U); // receiving does not clear the counter
}

TEST_F(CanZephyrBackend, CloseAndReopenDiscardsQueuedFramesAndCounters)
{
    zephyr::basic_backend<2U, 4U> backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), h, nullptr), status::ok);
    for (uint32_t id = 1U; id <= 4U; ++id) // two queued, two overruns
    {
        EXPECT_EQ(fake_can.deliver(&dev_, native_frame(id, 0U, 0U)), 1);
    }
    fake_can.tx_callback = nullptr;
    ASSERT_EQ(backend.send(frame(0x1U), no_wait, nullptr), status::ok);
    fake_can.tx_callback(&dev_, -EIO, fake_can.tx_user_data);

    ASSERT_EQ(backend.close(nullptr), status::ok);
    EXPECT_EQ(fake_can.installed_filters(), 0);
    open_ok(backend);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.rx_overruns, 0U);
    EXPECT_EQ(st.tx_failures, 0U);

    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty); // old frames are gone
    ASSERT_EQ(backend.add_filter(filter::any(), h, nullptr), status::ok);
    EXPECT_EQ(h, 0); // filter table restarted
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x20U, 0U, 1U, 0x66U)), 1);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x20U);
    EXPECT_EQ(out.data[0], 0x66U);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
}

TEST_F(CanZephyrBackend, CloseWithQueuedFramesLeavesNothingBehind)
{
    zephyr::backend backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), h, nullptr), status::ok);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x1U, 0U, 0U)), 1);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x2U, 0U, 0U)), 1);
    ASSERT_EQ(backend.close(nullptr), status::ok);

    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::not_configured);
    open_ok(backend);
    ASSERT_EQ(backend.add_filter(filter::any(), h, nullptr), status::ok);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x3U, 0U, 0U)), 1);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x3U);
}

TEST_F(CanZephyrBackend, OverlappingFiltersDeliverOncePerMatchingFilter)
{
    zephyr::backend backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), h, nullptr), status::ok);
    ASSERT_EQ(backend.add_filter(filter::exact(0x55U), h, nullptr), status::ok);

    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x55U, 0U, 1U, 7U)), 2);
    frame out;
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x55U);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
}

// ---------------------------------------------------------------------------------------------
// filters
// ---------------------------------------------------------------------------------------------

TEST_F(CanZephyrBackend, FilterOperationsOnClosedBackendAreNotConfigured)
{
    zephyr::backend backend;
    filter_handle h = 99;
    int err = 2;
    EXPECT_EQ(backend.add_filter(filter::any(), h, &err), status::not_configured);
    EXPECT_EQ(h, invalid_filter);
    EXPECT_EQ(err, 0);
    err = 2;
    EXPECT_EQ(backend.remove_filter(0, &err), status::not_configured);
    EXPECT_EQ(err, 0);
    err = 2;
    EXPECT_EQ(backend.clear_filters(&err), status::not_configured);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(fake_can.remove_calls, 0);
}

TEST_F(CanZephyrBackend, AddFilterForwardsFilterToDriver)
{
    zephyr::backend backend;
    open_ok(backend);

    filter_handle h = 99;
    int err = 4;
    ASSERT_EQ(backend.add_filter(filter(0x120U, 0x7F0U), h, &err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(h, 0);
    ASSERT_TRUE(fake_can.filters[0].used);
    EXPECT_EQ(fake_can.filters[0].filter.id, 0x120U);
    EXPECT_EQ(fake_can.filters[0].filter.mask, 0x7F0U);
    EXPECT_EQ(fake_can.filters[0].filter.flags, 0U);
    EXPECT_EQ(fake_can.filters[0].user_data, static_cast<void*>(&backend));
    EXPECT_NE(fake_can.filters[0].callback, nullptr);

    ASSERT_EQ(backend.add_filter(filter::exact(0x18FF0001U, id_format::extended), h, nullptr), status::ok);
    ASSERT_TRUE(fake_can.filters[1].used);
    EXPECT_EQ(fake_can.filters[1].filter.id, 0x18FF0001U);
    EXPECT_EQ(fake_can.filters[1].filter.flags, CAN_FILTER_IDE);
}

TEST_F(CanZephyrBackend, ExtendedAndStandardFiltersMatchOnlyTheirFormat)
{
    zephyr::backend backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter(0x120U, 0x7F0U), h, nullptr), status::ok);
    ASSERT_EQ(backend.add_filter(filter::exact(0x123U, id_format::extended), h, nullptr), status::ok);

    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x125U, 0U, 0U)), 1);        // standard block
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x135U, 0U, 0U)), 0);        // outside the block
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x125U, CAN_FRAME_IDE, 0U)), 0); // wrong format
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x123U, CAN_FRAME_IDE, 0U)), 1); // extended exact
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x123U, 0U, 0U)), 1);        // standard block also covers 0x123
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x124U, CAN_FRAME_IDE, 0U)), 0);

    frame out;
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x125U);
    EXPECT_FALSE(out.is_extended());
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x123U);
    EXPECT_TRUE(out.is_extended());
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x123U);
    EXPECT_FALSE(out.is_extended());
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
}

TEST_F(CanZephyrBackend, AddFilterUntilMaxFiltersIsFull)
{
    zephyr::basic_backend<4U, 3U> backend;
    open_ok(backend);
    filter_handle h = invalid_filter;
    for (int i = 0; i < 3; ++i)
    {
        ASSERT_EQ(backend.add_filter(filter::exact(static_cast<uint32_t>(0x10 + i)), h, nullptr), status::ok);
        EXPECT_EQ(h, i);
    }
    int err = 9;
    h = 42;
    EXPECT_EQ(backend.add_filter(filter::exact(0x99U), h, &err), status::full);
    EXPECT_EQ(h, invalid_filter);
    EXPECT_EQ(err, 0); // software limit, not a driver errno
    EXPECT_EQ(fake_can.installed_filters(), 3);

    ASSERT_EQ(backend.remove_filter(1, nullptr), status::ok);
    ASSERT_EQ(backend.add_filter(filter::exact(0x99U), h, nullptr), status::ok);
    EXPECT_EQ(h, 1);
}

TEST_F(CanZephyrBackend, ControllerOutOfFilterSlotsMapsToFull)
{
    zephyr::basic_backend<4U, 6U> backend; // more filters than the fake controller has slots (4)
    open_ok(backend);
    filter_handle h = invalid_filter;
    for (int i = 0; i < fake_slots(); ++i)
    {
        ASSERT_EQ(backend.add_filter(filter::exact(static_cast<uint32_t>(i)), h, nullptr), status::ok);
    }

    int err = 0;
    h = 17;
    EXPECT_EQ(backend.add_filter(filter::exact(0x7FFU), h, &err), status::full);
    EXPECT_EQ(err, ENOSPC);
    EXPECT_EQ(h, invalid_filter);
    EXPECT_EQ(fake_can.installed_filters(), fake_slots());

    // Nothing was recorded for the failed add: the next handle is still the lowest free one.
    ASSERT_EQ(backend.remove_filter(2, nullptr), status::ok);
    ASSERT_EQ(backend.add_filter(filter::exact(0x7FFU), h, nullptr), status::ok);
    EXPECT_EQ(h, 2);
}

TEST_F(CanZephyrBackend, DriverRejectedFilterIsInvalidConfigAndNotKept)
{
    zephyr::basic_backend<4U, 2U> backend;
    open_ok(backend);
    filter_handle h = 5;
    int err = 0;
    fake_can.add_filter_result = -ENOTSUP;
    EXPECT_EQ(backend.add_filter(filter(0x100U, 0x700U), h, &err), status::invalid_config);
    EXPECT_EQ(err, ENOTSUP);
    EXPECT_EQ(h, invalid_filter);
    EXPECT_EQ(fake_can.installed_filters(), 0);

    fake_can.add_filter_result = -EINVAL;
    EXPECT_EQ(backend.add_filter(filter(0x100U, 0x700U), h, &err), status::invalid_argument);
    EXPECT_EQ(err, EINVAL);

    // The failures consumed no software slot: both slots are still available.
    fake_can.add_filter_result = 0;
    ASSERT_EQ(backend.add_filter(filter::exact(1U), h, nullptr), status::ok);
    EXPECT_EQ(h, 0);
    ASSERT_EQ(backend.add_filter(filter::exact(2U), h, nullptr), status::ok);
    EXPECT_EQ(h, 1);
    EXPECT_EQ(backend.add_filter(filter::exact(3U), h, nullptr), status::full);
}

TEST_F(CanZephyrBackend, RemoveFilterUsesDriverIdNotHandle)
{
    zephyr::backend backend;
    open_ok(backend);
    // A foreign user of the controller already owns driver slot 0.
    fake_can.filters[0].used = true;
    fake_can.filters[0].callback = &ignore_frame;

    filter_handle h = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::exact(0x10U), h, nullptr), status::ok);
    EXPECT_EQ(h, 0); // castle handle
    ASSERT_TRUE(fake_can.filters[1].used); // driver id 1
    EXPECT_EQ(fake_can.filters[1].filter.id, 0x10U);

    EXPECT_EQ(backend.remove_filter(h, nullptr), status::ok);
    EXPECT_EQ(fake_can.remove_calls, 1);
    EXPECT_FALSE(fake_can.filters[1].used);
    EXPECT_TRUE(fake_can.filters[0].used); // foreign filter untouched

    // clear_filters and close go through the same id mapping.
    ASSERT_EQ(backend.add_filter(filter::exact(0x11U), h, nullptr), status::ok);
    ASSERT_EQ(backend.clear_filters(nullptr), status::ok);
    EXPECT_FALSE(fake_can.filters[1].used);
    EXPECT_TRUE(fake_can.filters[0].used);
    ASSERT_EQ(backend.add_filter(filter::exact(0x12U), h, nullptr), status::ok);
    ASSERT_EQ(backend.close(nullptr), status::ok);
    EXPECT_FALSE(fake_can.filters[1].used);
    EXPECT_TRUE(fake_can.filters[0].used);
}

TEST_F(CanZephyrBackend, RemoveFilterHandlesStaleAndInvalidHandles)
{
    zephyr::basic_backend<4U, 3U> backend;
    open_ok(backend);
    filter_handle a = invalid_filter;
    filter_handle b = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::exact(0x1U), a, nullptr), status::ok);
    ASSERT_EQ(backend.add_filter(filter::exact(0x2U), b, nullptr), status::ok);

    int err = 3;
    EXPECT_EQ(backend.remove_filter(a, &err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(fake_can.remove_calls, 1);

    EXPECT_EQ(backend.remove_filter(a, &err), status::not_found); // stale
    EXPECT_EQ(err, 0);
    EXPECT_EQ(backend.remove_filter(2, nullptr), status::not_found); // never allocated
    EXPECT_EQ(backend.remove_filter(invalid_filter, nullptr), status::not_found);
    EXPECT_EQ(backend.remove_filter(3, nullptr), status::not_found); // == MaxFilters
    EXPECT_EQ(backend.remove_filter(1000, nullptr), status::not_found);
    EXPECT_EQ(fake_can.remove_calls, 1); // driver was never asked again

    // The surviving filter still delivers; the removed one does not.
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x1U, 0U, 0U)), 0);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x2U, 0U, 0U)), 1);

    // Handle reuse: the freed lowest slot comes back first.
    filter_handle c = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::exact(0x3U), c, nullptr), status::ok);
    EXPECT_EQ(c, a);
}

TEST_F(CanZephyrBackend, ClearFiltersRemovesEverythingAndRestartsHandles)
{
    zephyr::basic_backend<4U, 4U> backend;
    open_ok(backend);
    filter_handle h0 = invalid_filter;
    filter_handle h1 = invalid_filter;
    filter_handle h2 = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), h0, nullptr), status::ok);
    ASSERT_EQ(backend.add_filter(filter::any(id_format::extended), h1, nullptr), status::ok);
    ASSERT_EQ(backend.add_filter(filter::exact(0x5U), h2, nullptr), status::ok);
    ASSERT_EQ(backend.remove_filter(h1, nullptr), status::ok); // sparse table
    ASSERT_EQ(fake_can.remove_calls, 1);

    int err = 6;
    EXPECT_EQ(backend.clear_filters(&err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(fake_can.remove_calls, 3); // only the two still-installed filters
    EXPECT_EQ(fake_can.installed_filters(), 0);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x5U, 0U, 0U)), 0);

    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
    EXPECT_EQ(backend.remove_filter(h0, nullptr), status::not_found);
    EXPECT_EQ(backend.remove_filter(h2, nullptr), status::not_found);

    EXPECT_EQ(backend.clear_filters(nullptr), status::ok); // idempotent
    EXPECT_EQ(fake_can.remove_calls, 3);

    filter_handle fresh = invalid_filter;
    ASSERT_EQ(backend.add_filter(filter::any(), fresh, nullptr), status::ok);
    EXPECT_EQ(fresh, 0);
}

// ---------------------------------------------------------------------------------------------
// query_status
// ---------------------------------------------------------------------------------------------

TEST_F(CanZephyrBackend, QueryStatusOnClosedBackendIsNotConfiguredWithDefaultOut)
{
    zephyr::backend backend;
    bus_status st;
    st.state = bus_state::bus_off;
    st.tx_error_counter = 9U;
    st.rx_overruns = 4U;
    int err = 3;
    EXPECT_EQ(backend.query_status(st, &err), status::not_configured);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(st.state, bus_state::stopped);
    EXPECT_EQ(st.tx_error_counter, 0U);
    EXPECT_EQ(st.rx_error_counter, 0U);
    EXPECT_EQ(st.rx_overruns, 0U);
    EXPECT_EQ(st.tx_failures, 0U);
}

TEST_F(CanZephyrBackend, QueryStatusReportsEveryStateAndCounters)
{
    zephyr::backend backend;
    open_ok(backend);

    struct row
    {
        enum can_state native;
        bus_state expected;
    };
    const row rows[] = {
        {CAN_STATE_ERROR_ACTIVE, bus_state::error_active},
        {CAN_STATE_ERROR_WARNING, bus_state::error_warning},
        {CAN_STATE_ERROR_PASSIVE, bus_state::error_passive},
        {CAN_STATE_BUS_OFF, bus_state::bus_off},
        {CAN_STATE_STOPPED, bus_state::stopped},
    };
    uint8_t tx = 10U;
    for (const row& r : rows)
    {
        fake_can.state = r.native;
        fake_can.counters = can_bus_err_cnt{tx, static_cast<uint8_t>(tx + 1U)};
        bus_status st;
        int err = 8;
        ASSERT_EQ(backend.query_status(st, &err), status::ok);
        EXPECT_EQ(err, 0);
        EXPECT_EQ(st.state, r.expected);
        EXPECT_EQ(st.tx_error_counter, tx);
        EXPECT_EQ(st.rx_error_counter, tx + 1U);
        EXPECT_EQ(st.rx_overruns, 0U);
        EXPECT_EQ(st.tx_failures, 0U);
        tx = static_cast<uint8_t>(tx + 60U);
    }
}

TEST_F(CanZephyrBackend, QueryStatusMapsDriverFailureAndResetsOut)
{
    zephyr::backend backend;
    open_ok(backend);
    fake_can.get_state_result = -EIO;
    bus_status st;
    st.state = bus_state::error_passive;
    st.rx_error_counter = 77U;
    int err = 0;
    EXPECT_EQ(backend.query_status(st, &err), status::system_call_error);
    EXPECT_EQ(err, EIO);
    EXPECT_EQ(st.state, bus_state::stopped);
    EXPECT_EQ(st.rx_error_counter, 0U);

    fake_can.get_state_result = -ENETDOWN;
    EXPECT_EQ(backend.query_status(st, &err), status::not_configured);
    EXPECT_EQ(err, ENETDOWN);

    fake_can.get_state_result = 0;
    EXPECT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.state, bus_state::error_active);
}

// ---------------------------------------------------------------------------------------------
// basic_bus facade end-to-end
// ---------------------------------------------------------------------------------------------

TEST_F(CanZephyrBackend, BusEndToEnd)
{
    default_zephyr_bus bus;
    EXPECT_FALSE(bus.is_open());

    int err = 1;
    ASSERT_EQ(bus.open(&dev_, can_config(bitrate_500k, can_mode::loopback), &err), status::ok);
    EXPECT_EQ(err, 0);
    EXPECT_TRUE(bus.is_open());
    EXPECT_EQ(fake_can.bitrate, bitrate_500k);
    EXPECT_EQ(fake_can.mode, CAN_MODE_LOOPBACK);
    EXPECT_EQ(bus.open(&dev_), status::invalid_config); // already open

    ASSERT_EQ(bus.accept_all(), status::ok);
    EXPECT_EQ(fake_can.installed_filters(), 2);

    frame out;
    EXPECT_EQ(bus.receive(out), status::empty);

    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x123U, 0U, 3U, 0x01U)), 1);
    EXPECT_EQ(fake_can.deliver(&dev_, native_frame(0x1ABCDE00U, CAN_FRAME_IDE, 1U, 0x09U)), 1);
    ASSERT_EQ(bus.receive(out, timeout_type(100)), status::ok);
    EXPECT_EQ(out, data_frame(0x123U, id_format::standard, 0x01U, 3U));
    ASSERT_EQ(bus.receive(out, wait_forever), status::ok);
    EXPECT_EQ(out, data_frame(0x1ABCDE00U, id_format::extended, 0x09U, 1U));

    const uint8_t payload[] = {0xDEU, 0xADU, 0xBEU, 0xEFU};
    ASSERT_EQ(bus.send(0x7E0U, payload, sizeof(payload), id_format::standard, timeout_type(50), &err), status::ok);
    EXPECT_EQ(fake_can.sent_frame.id, 0x7E0U);
    EXPECT_EQ(fake_can.sent_frame.dlc, 4U);
    EXPECT_EQ(fake_can.sent_frame.data[3], 0xEFU);
    EXPECT_EQ(fake_can.send_timeout.ticks, 50);
    ASSERT_EQ(bus.send(data_frame(0x1U, id_format::extended, 0U, 0U)), status::ok);
    EXPECT_EQ(fake_can.sent_frame.flags, CAN_FRAME_IDE);
    EXPECT_EQ(fake_can.send_timeout.ticks, K_NO_WAIT.ticks);

    fake_can.state = CAN_STATE_ERROR_WARNING;
    fake_can.counters = can_bus_err_cnt{100U, 3U};
    fake_can.tx_callback(&dev_, -EIO, fake_can.tx_user_data);
    bus_status st;
    ASSERT_EQ(bus.query_status(st), status::ok);
    EXPECT_EQ(st.state, bus_state::error_warning);
    EXPECT_EQ(st.tx_error_counter, 100U);
    EXPECT_EQ(st.rx_error_counter, 3U);
    EXPECT_EQ(st.tx_failures, 1U);
    EXPECT_EQ(st.rx_overruns, 0U);

    ASSERT_EQ(bus.clear_filters(), status::ok);
    EXPECT_EQ(fake_can.installed_filters(), 0);

    ASSERT_EQ(bus.close(), status::ok);
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.close(), status::not_configured);
    EXPECT_EQ(bus.receive(out), status::not_configured);
    EXPECT_EQ(bus.query_status(st), status::not_configured);
    EXPECT_EQ(&bus.backend(), &static_cast<const default_zephyr_bus&>(bus).backend());
}

TEST_F(CanZephyrBackend, BusFacadeValidatesBeforeTouchingTheDriver)
{
    default_zephyr_bus bus;
    EXPECT_EQ(bus.open(&dev_, can_config(max_bitrate + 1U)), status::invalid_config);
    EXPECT_EQ(bus.open(&dev_, can_config(0U, static_cast<can_mode>(9U))), status::invalid_config);
    EXPECT_EQ(fake_can.stop_calls, 0);
    EXPECT_EQ(bus.open(nullptr), status::invalid_argument);
    ASSERT_EQ(bus.open(&dev_), status::ok);

    EXPECT_EQ(bus.send(frame(0x800U), no_wait), status::invalid_argument); // 12-bit standard id
    EXPECT_EQ(bus.send(frame(0x1U), timeout_type(-1)), status::invalid_argument);
    frame out;
    EXPECT_EQ(bus.receive(out, timeout_type(-1)), status::invalid_argument);
    EXPECT_EQ(bus.add_filter(filter(0x800U, 0U)), status::invalid_argument);
    EXPECT_EQ(fake_can.send_calls, 0);
    EXPECT_EQ(fake_can.installed_filters(), 0);
}

TEST_F(CanZephyrBackend, BusAcceptAllRollsBackFirstFilterWhenSecondFails)
{
    basic_bus<zephyr::basic_backend<4U, 1U>> bus;
    ASSERT_EQ(bus.open(&dev_), status::ok);
    EXPECT_EQ(bus.accept_all(), status::full);
    EXPECT_EQ(fake_can.installed_filters(), 0);
    EXPECT_EQ(fake_can.remove_calls, 1);

    filter_handle h = invalid_filter;
    EXPECT_EQ(bus.add_filter(filter::any(), &h), status::ok);
    EXPECT_EQ(h, 0);
}

} // namespace
