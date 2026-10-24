#include <gtest/gtest.h>

#include "castle/core/traits.hpp"
#include "castle_ext/io/can/backends/mock.hpp"
#include "castle_ext/io/can/bus.hpp"

namespace
{

using namespace castle::io::can;
using castle::status;
using castle::chrono::milliseconds;

constexpr int untouched = 123;

/**
 * Scriptable backend: records every call and returns a configured status, so the facade's
 * validation, errno handling and pass-through can be observed without any real controller.
 */
struct probe_backend
{
    using endpoint_type = int;

    int open_calls = 0;
    int close_calls = 0;
    int send_calls = 0;
    int receive_calls = 0;
    int add_calls = 0;
    int remove_calls = 0;
    int clear_calls = 0;
    int query_calls = 0;
    int total_calls() const
    {
        return open_calls + close_calls + send_calls + receive_calls + add_calls + remove_calls + clear_calls
             + query_calls;
    }

    status result = status::ok;      // returned by every call (add_filter: while budget lasts)
    int set_errno = -1;              // written to *system_error by every call when >= 0
    bool opened = false;
    int add_budget = 1000;           // add_filter calls that still succeed
    status add_exhausted = status::full;
    filter_handle handle_to_give = 7;

    endpoint_type last_endpoint = -1;
    can_config last_config;
    frame last_sent;
    timeout_type last_timeout = no_wait;
    filter last_filter;
    filter_handle last_removed = invalid_filter;
    frame next_received = frame(0x321U, id_format::extended);

    void touch(int* system_error) const
    {
        if (system_error != nullptr && set_errno >= 0)
        {
            *system_error = set_errno;
        }
    }

    status open(endpoint_type endpoint, const can_config& config, int* system_error)
    {
        ++open_calls;
        last_endpoint = endpoint;
        last_config = config;
        touch(system_error);
        opened = (result == status::ok);
        return result;
    }

    status close(int* system_error)
    {
        ++close_calls;
        touch(system_error);
        return result;
    }

    bool is_open() const { return opened; }

    status send(const frame& value, timeout_type timeout, int* system_error)
    {
        ++send_calls;
        last_sent = value;
        last_timeout = timeout;
        touch(system_error);
        return result;
    }

    status receive(frame& out, timeout_type timeout, int* system_error)
    {
        ++receive_calls;
        last_timeout = timeout;
        touch(system_error);
        if (result == status::ok)
        {
            out = next_received;
        }
        return result;
    }

    status add_filter(const filter& value, filter_handle& handle, int* system_error)
    {
        ++add_calls;
        last_filter = value;
        handle = handle_to_give; // deliberately also written on failure
        touch(system_error);
        if (result != status::ok)
        {
            return result;
        }
        if (add_budget <= 0)
        {
            return add_exhausted;
        }
        --add_budget;
        return status::ok;
    }

    status remove_filter(filter_handle handle, int* system_error)
    {
        ++remove_calls;
        last_removed = handle;
        touch(system_error);
        return result;
    }

    status clear_filters(int* system_error)
    {
        ++clear_calls;
        touch(system_error);
        return result;
    }

    status query_status(bus_status& out, int* system_error)
    {
        ++query_calls;
        touch(system_error);
        out.state = bus_state::error_passive;
        out.tx_error_counter = 11U;
        return result;
    }
};

using probe_bus = basic_bus<probe_backend>;

frame make_frame(uint32_t id, id_format format = id_format::standard)
{
    return frame(id, format);
}

// ---------------------------------------------------------------- backend contract detection

struct missing_query_status
{
    using endpoint_type = int;
    status open(int, const can_config&, int*) { return status::ok; }
    status close(int*) { return status::ok; }
    bool is_open() const { return false; }
    status send(const frame&, timeout_type, int*) { return status::ok; }
    status receive(frame&, timeout_type, int*) { return status::ok; }
    status add_filter(const filter&, filter_handle&, int*) { return status::ok; }
    status remove_filter(filter_handle, int*) { return status::ok; }
    status clear_filters(int*) { return status::ok; }
};

struct missing_endpoint_type
{
    status open(int, const can_config&, int*) { return status::ok; }
    status close(int*) { return status::ok; }
    bool is_open() const { return false; }
    status send(const frame&, timeout_type, int*) { return status::ok; }
    status receive(frame&, timeout_type, int*) { return status::ok; }
    status add_filter(const filter&, filter_handle&, int*) { return status::ok; }
    status remove_filter(filter_handle, int*) { return status::ok; }
    status clear_filters(int*) { return status::ok; }
    status query_status(bus_status&, int*) { return status::ok; }
};

struct non_const_is_open
{
    using endpoint_type = int;
    status open(int, const can_config&, int*) { return status::ok; }
    status close(int*) { return status::ok; }
    bool is_open() { return false; } // contract demands a const member
    status send(const frame&, timeout_type, int*) { return status::ok; }
    status receive(frame&, timeout_type, int*) { return status::ok; }
    status add_filter(const filter&, filter_handle&, int*) { return status::ok; }
    status remove_filter(filter_handle, int*) { return status::ok; }
    status clear_filters(int*) { return status::ok; }
    status query_status(bus_status&, int*) { return status::ok; }
};

struct wrong_open_signature
{
    using endpoint_type = int;
    status open(int) { return status::ok; }
    status close(int*) { return status::ok; }
    bool is_open() const { return false; }
    status send(const frame&, timeout_type, int*) { return status::ok; }
    status receive(frame&, timeout_type, int*) { return status::ok; }
    status add_filter(const filter&, filter_handle&, int*) { return status::ok; }
    status remove_filter(filter_handle, int*) { return status::ok; }
    status clear_filters(int*) { return status::ok; }
    status query_status(bus_status&, int*) { return status::ok; }
};

TEST(CanBus, BackendContractDetection)
{
    EXPECT_TRUE(detail::has_bus_backend_contract<mock::backend>::value);
    EXPECT_TRUE((detail::has_bus_backend_contract<mock::basic_backend<1U, 1U, 1U>>::value));
    EXPECT_TRUE(detail::has_bus_backend_contract<probe_backend>::value);

    EXPECT_FALSE(detail::has_bus_backend_contract<int>::value);
    EXPECT_FALSE(detail::has_bus_backend_contract<missing_query_status>::value);
    EXPECT_FALSE(detail::has_bus_backend_contract<missing_endpoint_type>::value);
    EXPECT_FALSE(detail::has_bus_backend_contract<non_const_is_open>::value);
    EXPECT_FALSE(detail::has_bus_backend_contract<wrong_open_signature>::value);
}

// ---------------------------------------------------------------- type properties

TEST(CanBus, AliasesAndTypedefs)
{
    EXPECT_TRUE((castle::meta::is_same<bus<mock::backend>, basic_bus<mock::backend>>::value));
    EXPECT_TRUE((castle::meta::is_same<probe_bus::backend_type, probe_backend>::value));
    EXPECT_TRUE((castle::meta::is_same<probe_bus::endpoint_type, int>::value));
    EXPECT_TRUE((castle::meta::is_same<basic_bus<mock::backend>::endpoint_type, const char*>::value));
}

TEST(CanBus, FacadeAddsNoStorage)
{
    EXPECT_EQ(sizeof(basic_bus<mock::backend>), sizeof(mock::backend));
    EXPECT_EQ(sizeof(basic_bus<probe_backend>), sizeof(probe_backend));
    EXPECT_EQ((sizeof(basic_bus<mock::basic_backend<2U, 3U, 4U>>)), (sizeof(mock::basic_backend<2U, 3U, 4U>)));
}

TEST(CanBus, NonCopyableButMovesWhenBackendMoves)
{
    EXPECT_FALSE(castle::meta::is_copy_constructible<probe_bus>::value);
    EXPECT_FALSE(castle::meta::is_copy_assignable<probe_bus>::value);
    EXPECT_TRUE(castle::meta::is_move_constructible<probe_bus>::value);
    EXPECT_TRUE(castle::meta::is_move_assignable<probe_bus>::value);
    EXPECT_TRUE(castle::meta::is_default_constructible<probe_bus>::value);

    // The mock owns non-movable ring buffers, so the facade inherits that.
    EXPECT_FALSE(castle::meta::is_copy_constructible<basic_bus<mock::backend>>::value);
    EXPECT_EQ(castle::meta::is_move_constructible<basic_bus<mock::backend>>::value,
              castle::meta::is_move_constructible<mock::backend>::value);
    EXPECT_EQ(castle::meta::is_move_assignable<basic_bus<mock::backend>>::value,
              castle::meta::is_move_assignable<mock::backend>::value);
}

TEST(CanBus, MoveTransfersBackendState)
{
    probe_bus source;
    ASSERT_EQ(source.open(5, can_config(bitrate_125k, can_mode::loopback)), status::ok);

    probe_bus moved(static_cast<probe_bus&&>(source));
    EXPECT_TRUE(moved.is_open());
    EXPECT_EQ(moved.backend().last_endpoint, 5);
    EXPECT_EQ(moved.backend().last_config.bitrate, bitrate_125k);
    EXPECT_EQ(moved.backend().open_calls, 1);

    probe_bus assigned;
    EXPECT_FALSE(assigned.is_open());
    assigned = static_cast<probe_bus&&>(moved);
    EXPECT_TRUE(assigned.is_open());
    EXPECT_EQ(assigned.backend().last_config.mode, can_mode::loopback);
}

TEST(CanBus, BackendAccessorsReturnTheOwnedBackend)
{
    probe_bus bus;
    const probe_bus& view = bus;

    EXPECT_TRUE((castle::meta::is_same<decltype(bus.backend()), probe_backend&>::value));
    EXPECT_TRUE((castle::meta::is_same<decltype(view.backend()), const probe_backend&>::value));
    EXPECT_EQ(&bus.backend(), &view.backend());

    bus.backend().result = status::unknown_error;
    EXPECT_EQ(view.backend().result, status::unknown_error);
    bus.backend().opened = true;
    EXPECT_TRUE(bus.is_open());
    EXPECT_TRUE(view.is_open());
}

// ---------------------------------------------------------------- open

TEST(CanBus, OpenForwardsEndpointAndConfig)
{
    probe_bus bus;
    EXPECT_FALSE(bus.is_open());
    ASSERT_EQ(bus.open(42, can_config(bitrate_800k, can_mode::listen_only)), status::ok);
    EXPECT_TRUE(bus.is_open());
    EXPECT_EQ(bus.backend().open_calls, 1);
    EXPECT_EQ(bus.backend().last_endpoint, 42);
    EXPECT_EQ(bus.backend().last_config.bitrate, bitrate_800k);
    EXPECT_EQ(bus.backend().last_config.mode, can_mode::listen_only);
}

TEST(CanBus, OpenDefaultsToDefaultConfig)
{
    probe_bus bus;
    ASSERT_EQ(bus.open(1), status::ok);
    EXPECT_EQ(bus.backend().last_config.bitrate, 0U);
    EXPECT_EQ(bus.backend().last_config.mode, can_mode::normal);
}

TEST(CanBus, OpenAcceptsBoundaryConfigs)
{
    probe_bus bus;
    EXPECT_EQ(bus.open(1, can_config(0U, can_mode::normal)), status::ok);
    EXPECT_EQ(bus.open(1, can_config(max_bitrate, can_mode::listen_only)), status::ok);
    EXPECT_EQ(bus.open(1, can_config(bitrate_1m, can_mode::loopback)), status::ok);
    EXPECT_EQ(bus.backend().open_calls, 3);
}

TEST(CanBus, OpenRejectsInvalidConfigWithoutTouchingBackend)
{
    probe_bus bus;
    int error = untouched;
    EXPECT_EQ(bus.open(1, can_config(max_bitrate + 1U), &error), status::invalid_config);
    EXPECT_EQ(error, 0);
    error = untouched;
    EXPECT_EQ(bus.open(1, can_config(0xFFFFFFFFU), &error), status::invalid_config);
    EXPECT_EQ(error, 0);
    error = untouched;
    EXPECT_EQ(bus.open(1, can_config(bitrate_500k, static_cast<can_mode>(3)), &error), status::invalid_config);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.open(1, can_config(0U, static_cast<can_mode>(255))), status::invalid_config);

    EXPECT_EQ(bus.backend().total_calls(), 0);
    EXPECT_FALSE(bus.is_open());
}

TEST(CanBus, OpenClearsSystemErrorThenLetsBackendSetIt)
{
    probe_bus bus;
    int error = untouched;
    ASSERT_EQ(bus.open(1, can_config(), &error), status::ok); // probe leaves error alone
    EXPECT_EQ(error, 0);

    bus.backend().set_errno = 19;
    bus.backend().result = status::system_call_error;
    error = untouched;
    EXPECT_EQ(bus.open(1, can_config(), &error), status::system_call_error);
    EXPECT_EQ(error, 19);
}

// ---------------------------------------------------------------- close / is_open

TEST(CanBus, CloseForwardsAndClearsSystemError)
{
    probe_bus bus;
    int error = untouched;
    EXPECT_EQ(bus.close(&error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().close_calls, 1);

    bus.backend().result = status::not_configured;
    EXPECT_EQ(bus.close(), status::not_configured);
    EXPECT_EQ(bus.backend().close_calls, 2);
}

TEST(CanBus, OpenCloseLifecycleOverMock)
{
    basic_bus<mock::backend> bus;
    EXPECT_FALSE(bus.is_open());
    ASSERT_EQ(bus.open("vcan0"), status::ok);
    EXPECT_TRUE(bus.is_open());
    EXPECT_EQ(bus.open("vcan0"), status::invalid_config);
    EXPECT_EQ(bus.close(), status::ok);
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.close(), status::not_configured);
    ASSERT_EQ(bus.open("vcan0"), status::ok);
    EXPECT_TRUE(bus.is_open());
}

// ---------------------------------------------------------------- send(frame)

TEST(CanBus, SendForwardsValidFrameAndTimeout)
{
    probe_bus bus;
    frame value(0x7FFU);
    const uint8_t payload[4] = {1, 2, 3, 4};
    ASSERT_EQ(value.set_data(payload, 4U), status::ok);

    int error = untouched;
    ASSERT_EQ(bus.send(value, milliseconds(25), &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().send_calls, 1);
    EXPECT_EQ(bus.backend().last_sent, value);
    EXPECT_EQ(bus.backend().last_timeout.count(), 25);

    ASSERT_EQ(bus.send(value), status::ok); // default timeout is no_wait
    EXPECT_EQ(bus.backend().last_timeout.count(), 0);
    ASSERT_EQ(bus.send(value, wait_forever), status::ok);
    EXPECT_TRUE(is_forever(bus.backend().last_timeout));
    EXPECT_EQ(bus.backend().send_calls, 3);
}

TEST(CanBus, SendAcceptsBoundaryFrames)
{
    probe_bus bus;
    EXPECT_EQ(bus.send(make_frame(max_standard_id)), status::ok);
    EXPECT_EQ(bus.send(make_frame(max_extended_id, id_format::extended)), status::ok);
    frame remote(0U);
    ASSERT_EQ(remote.set_remote(8U), status::ok);
    EXPECT_EQ(bus.send(remote), status::ok);
    EXPECT_EQ(bus.send(make_frame(0U), milliseconds(0)), status::ok);
    EXPECT_EQ(bus.backend().send_calls, 4);
}

TEST(CanBus, SendRejectsInvalidFramesWithoutTouchingBackend)
{
    probe_bus bus;
    int error = untouched;

    frame bad_dlc(0x10U);
    bad_dlc.dlc = 9U;
    EXPECT_EQ(bus.send(bad_dlc, no_wait, &error), status::invalid_argument);
    EXPECT_EQ(error, 0);

    error = untouched;
    EXPECT_EQ(bus.send(make_frame(max_standard_id + 1U), no_wait, &error), status::invalid_argument);
    EXPECT_EQ(error, 0);

    error = untouched;
    EXPECT_EQ(bus.send(make_frame(max_extended_id + 1U, id_format::extended), no_wait, &error),
              status::invalid_argument);
    EXPECT_EQ(error, 0);

    frame bad_format(0x10U);
    bad_format.format = static_cast<id_format>(2);
    EXPECT_EQ(bus.send(bad_format), status::invalid_argument);

    frame bad_type(0x10U);
    bad_type.type = static_cast<frame_type>(2);
    EXPECT_EQ(bus.send(bad_type), status::invalid_argument);

    EXPECT_EQ(bus.backend().send_calls, 0);
    EXPECT_EQ(bus.backend().total_calls(), 0);
}

TEST(CanBus, SendRejectsNegativeTimeoutWithoutTouchingBackend)
{
    probe_bus bus;
    int error = untouched;
    EXPECT_EQ(bus.send(make_frame(1U), milliseconds(-1), &error), status::invalid_argument);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.send(make_frame(1U), milliseconds(-100000)), status::invalid_argument);
    EXPECT_EQ(bus.backend().total_calls(), 0);
}

// ---------------------------------------------------------------- send(id, bytes, ...)

TEST(CanBus, SendBytesBuildsStandardDataFrame)
{
    probe_bus bus;
    const uint8_t payload[3] = {0xAAU, 0xBBU, 0xCCU};
    int error = untouched;
    ASSERT_EQ(bus.send(0x123U, payload, 3U, id_format::standard, milliseconds(5), &error), status::ok);
    EXPECT_EQ(error, 0);

    const frame& sent = bus.backend().last_sent;
    EXPECT_EQ(sent.id, 0x123U);
    EXPECT_FALSE(sent.is_extended());
    EXPECT_FALSE(sent.is_remote());
    EXPECT_EQ(sent.dlc, 3U);
    EXPECT_EQ(sent.data[0], 0xAAU);
    EXPECT_EQ(sent.data[2], 0xCCU);
    EXPECT_EQ(sent.data[3], 0U);
    EXPECT_EQ(bus.backend().last_timeout.count(), 5);

    // Defaults: standard format, no_wait.
    ASSERT_EQ(bus.send(0x7FFU, payload, 1U), status::ok);
    EXPECT_FALSE(bus.backend().last_sent.is_extended());
    EXPECT_EQ(bus.backend().last_timeout.count(), 0);
}

TEST(CanBus, SendBytesSupportsExtendedIdsAndEmptyPayload)
{
    probe_bus bus;
    ASSERT_EQ(bus.send(0x1FFFFFFFU, nullptr, 0U, id_format::extended), status::ok);
    EXPECT_TRUE(bus.backend().last_sent.is_extended());
    EXPECT_EQ(bus.backend().last_sent.id, 0x1FFFFFFFU);
    EXPECT_EQ(bus.backend().last_sent.dlc, 0U);

    const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_EQ(bus.send(0x800U, payload, 8U, id_format::extended), status::ok); // > 11 bit, fine when extended
    EXPECT_EQ(bus.backend().last_sent.dlc, 8U);
    EXPECT_EQ(bus.backend().last_sent.data[7], 8U);
    EXPECT_EQ(bus.backend().send_calls, 2);
}

TEST(CanBus, SendBytesRejectsBadLengthAndNullPayload)
{
    probe_bus bus;
    const uint8_t payload[9] = {};
    int error = untouched;
    EXPECT_EQ(bus.send(0x1U, payload, 9U, id_format::standard, no_wait, &error), status::out_of_range);
    EXPECT_EQ(error, 0);

    error = untouched;
    EXPECT_EQ(bus.send(0x1U, nullptr, 1U, id_format::standard, no_wait, &error), status::invalid_argument);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.send(0x1U, nullptr, 8U), status::invalid_argument);
    EXPECT_EQ(bus.backend().total_calls(), 0);
}

TEST(CanBus, SendBytesValidatesIdentifierAndTimeout)
{
    probe_bus bus;
    const uint8_t payload[1] = {0};
    int error = untouched;
    EXPECT_EQ(bus.send(max_standard_id + 1U, payload, 1U, id_format::standard, no_wait, &error),
              status::invalid_argument);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.send(max_extended_id + 1U, payload, 1U, id_format::extended), status::invalid_argument);
    EXPECT_EQ(bus.send(0x1U, payload, 1U, static_cast<id_format>(7)), status::invalid_argument);
    EXPECT_EQ(bus.send(0x1U, payload, 1U, id_format::standard, milliseconds(-1)), status::invalid_argument);
    EXPECT_EQ(bus.backend().total_calls(), 0);
}

// ---------------------------------------------------------------- receive

TEST(CanBus, ReceiveRejectsNegativeTimeoutWithoutTouchingBackend)
{
    probe_bus bus;
    frame out(0x55U);
    int error = untouched;
    EXPECT_EQ(bus.receive(out, milliseconds(-1), &error), status::invalid_argument);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(out, frame(0x55U));
    EXPECT_EQ(bus.backend().receive_calls, 0);
}

TEST(CanBus, ReceiveForwardsTimeoutAndDeliversFrame)
{
    probe_bus bus;
    frame out;
    int error = untouched;
    ASSERT_EQ(bus.receive(out, milliseconds(40), &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().last_timeout.count(), 40);
    EXPECT_EQ(out, bus.backend().next_received);

    ASSERT_EQ(bus.receive(out), status::ok); // default timeout is no_wait
    EXPECT_EQ(bus.backend().last_timeout.count(), 0);
    ASSERT_EQ(bus.receive(out, wait_forever), status::ok);
    EXPECT_TRUE(is_forever(bus.backend().last_timeout));
    EXPECT_EQ(bus.backend().receive_calls, 3);
}

// ---------------------------------------------------------------- add_filter

TEST(CanBus, AddFilterForwardsFilterAndReturnsHandle)
{
    probe_bus bus;
    bus.backend().handle_to_give = 3;
    const filter wanted(0x120U, 0x7F0U);
    filter_handle handle = 55;
    int error = untouched;
    ASSERT_EQ(bus.add_filter(wanted, &handle, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(handle, 3);
    EXPECT_EQ(bus.backend().last_filter.id, 0x120U);
    EXPECT_EQ(bus.backend().last_filter.mask, 0x7F0U);
    EXPECT_EQ(bus.backend().last_filter.format, id_format::standard);
}

TEST(CanBus, AddFilterAcceptsNullHandle)
{
    probe_bus bus;
    EXPECT_EQ(bus.add_filter(filter::any()), status::ok);
    EXPECT_EQ(bus.add_filter(filter::exact(0x1FFFFFFFU, id_format::extended), nullptr), status::ok);
    EXPECT_EQ(bus.backend().add_calls, 2);
    EXPECT_EQ(bus.backend().last_filter.format, id_format::extended);
}

TEST(CanBus, AddFilterRejectsInvalidFilterAndResetsHandle)
{
    probe_bus bus;
    filter_handle handle = 55;
    int error = untouched;

    EXPECT_EQ(bus.add_filter(filter(max_standard_id + 1U, 0U), &handle, &error), status::invalid_argument);
    EXPECT_EQ(handle, invalid_filter);
    EXPECT_EQ(error, 0);

    handle = 55;
    EXPECT_EQ(bus.add_filter(filter(0U, max_standard_id + 1U), &handle), status::invalid_argument);
    EXPECT_EQ(handle, invalid_filter);

    handle = 55;
    EXPECT_EQ(bus.add_filter(filter(max_extended_id + 1U, 0U, id_format::extended), &handle),
              status::invalid_argument);
    EXPECT_EQ(handle, invalid_filter);

    handle = 55;
    EXPECT_EQ(bus.add_filter(filter(0U, 0U, static_cast<id_format>(9)), &handle), status::invalid_argument);
    EXPECT_EQ(handle, invalid_filter);

    EXPECT_EQ(bus.add_filter(filter(max_standard_id + 1U, 0U), nullptr), status::invalid_argument);
    EXPECT_EQ(bus.backend().total_calls(), 0);
}

TEST(CanBus, AddFilterResetsHandleWhenBackendFails)
{
    probe_bus bus; // the probe writes handle 7 even when failing; the facade must not expose it
    bus.backend().result = status::full;
    filter_handle handle = 55;
    EXPECT_EQ(bus.add_filter(filter::any(), &handle), status::full);
    EXPECT_EQ(handle, invalid_filter);
    EXPECT_EQ(bus.backend().add_calls, 1);

    bus.backend().result = status::ok;
    bus.backend().add_budget = 0;
    handle = 55;
    EXPECT_EQ(bus.add_filter(filter::any(), &handle), status::full);
    EXPECT_EQ(handle, invalid_filter);
}

TEST(CanBus, AddFilterOverMockUntilFull)
{
    basic_bus<mock::basic_backend<4U, 4U, 2U>> bus;
    filter_handle handle = 55;
    EXPECT_EQ(bus.add_filter(filter::any(), &handle), status::not_configured);
    EXPECT_EQ(handle, invalid_filter);

    ASSERT_EQ(bus.open("vcan0"), status::ok);
    filter_handle first = invalid_filter;
    filter_handle second = invalid_filter;
    ASSERT_EQ(bus.add_filter(filter::exact(1U), &first), status::ok);
    ASSERT_EQ(bus.add_filter(filter::exact(2U), &second), status::ok);
    EXPECT_NE(first, invalid_filter);
    EXPECT_NE(second, invalid_filter);
    EXPECT_NE(first, second);

    handle = 55;
    int error = untouched;
    EXPECT_EQ(bus.add_filter(filter::exact(3U), &handle, &error), status::full);
    EXPECT_EQ(handle, invalid_filter);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().filter_count(), 2U);
}

// ---------------------------------------------------------------- accept_all

TEST(CanBus, AcceptAllInstallsExactlyTwoFilters)
{
    probe_bus bus;
    int error = untouched;
    ASSERT_EQ(bus.accept_all(&error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().add_calls, 2);
    EXPECT_EQ(bus.backend().remove_calls, 0);
    EXPECT_EQ(bus.backend().last_filter.format, id_format::extended);
    EXPECT_EQ(bus.backend().last_filter.id, 0U);
    EXPECT_EQ(bus.backend().last_filter.mask, 0U);
}

TEST(CanBus, AcceptAllOverMockReceivesStandardAndExtendedFrames)
{
    basic_bus<mock::backend> bus;
    ASSERT_EQ(bus.open("vcan0"), status::ok);
    int error = untouched;
    ASSERT_EQ(bus.accept_all(&error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().filter_count(), 2U);

    EXPECT_EQ(bus.backend().inject(make_frame(0x000U)), status::ok);
    EXPECT_EQ(bus.backend().inject(make_frame(0x7FFU)), status::ok);
    EXPECT_EQ(bus.backend().inject(make_frame(0x00000000U, id_format::extended)), status::ok);
    EXPECT_EQ(bus.backend().inject(make_frame(0x1FFFFFFFU, id_format::extended)), status::ok);

    frame out;
    ASSERT_EQ(bus.receive(out), status::ok);
    EXPECT_EQ(out.id, 0x000U);
    EXPECT_FALSE(out.is_extended());
    ASSERT_EQ(bus.receive(out), status::ok);
    EXPECT_EQ(out.id, 0x7FFU);
    ASSERT_EQ(bus.receive(out), status::ok);
    EXPECT_EQ(out.id, 0U);
    EXPECT_TRUE(out.is_extended());
    ASSERT_EQ(bus.receive(out), status::ok);
    EXPECT_EQ(out.id, 0x1FFFFFFFU);
    EXPECT_TRUE(out.is_extended());
    EXPECT_EQ(bus.receive(out), status::empty);
}

TEST(CanBus, AcceptAllRollsBackFirstFilterWhenSecondSlotMissing)
{
    basic_bus<mock::basic_backend<4U, 4U, 1U>> bus;
    ASSERT_EQ(bus.open("vcan0"), status::ok);

    int error = untouched;
    EXPECT_EQ(bus.accept_all(&error), status::full);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().filter_count(), 0U); // nothing left behind
    EXPECT_EQ(bus.backend().inject(make_frame(1U)), status::not_found);

    // The single slot is free again for a regular filter.
    filter_handle handle = invalid_filter;
    EXPECT_EQ(bus.add_filter(filter::any(), &handle), status::ok);
    EXPECT_EQ(handle, 0);
}

TEST(CanBus, AcceptAllWithOneSlotPreUsedKeepsPreExistingFilter)
{
    basic_bus<mock::basic_backend<4U, 4U, 2U>> bus;
    ASSERT_EQ(bus.open("vcan0"), status::ok);
    filter_handle pre = invalid_filter;
    ASSERT_EQ(bus.add_filter(filter::exact(0x42U), &pre), status::ok);
    ASSERT_EQ(bus.backend().filter_count(), 1U);

    EXPECT_EQ(bus.accept_all(), status::full);
    EXPECT_EQ(bus.backend().filter_count(), 1U);
    EXPECT_EQ(bus.backend().inject(make_frame(0x42U)), status::ok);
    EXPECT_EQ(bus.backend().inject(make_frame(0x43U)), status::not_found);
    EXPECT_EQ(bus.backend().inject(make_frame(0x42U, id_format::extended)), status::not_found);

    // Rolled-back slot is reusable.
    filter_handle again = invalid_filter;
    ASSERT_EQ(bus.add_filter(filter::any(), &again), status::ok);
    EXPECT_NE(again, pre);
    EXPECT_EQ(bus.backend().filter_count(), 2U);
}

TEST(CanBus, AcceptAllOnClosedMockReportsNotConfigured)
{
    basic_bus<mock::backend> bus;
    int error = untouched;
    EXPECT_EQ(bus.accept_all(&error), status::not_configured);
    EXPECT_EQ(error, 0);
}

TEST(CanBus, AcceptAllFirstFilterFailureSkipsSecondAndRollback)
{
    probe_bus bus;
    bus.backend().add_budget = 0;
    bus.backend().add_exhausted = status::invalid_config;
    EXPECT_EQ(bus.accept_all(), status::invalid_config);
    EXPECT_EQ(bus.backend().add_calls, 1);
    EXPECT_EQ(bus.backend().remove_calls, 0);
}

TEST(CanBus, AcceptAllSecondFilterFailureRollsBackFirstHandleAndKeepsError)
{
    probe_bus bus;
    bus.backend().add_budget = 1;
    bus.backend().add_exhausted = status::invalid_config;
    bus.backend().handle_to_give = 4;
    bus.backend().set_errno = 22;

    int error = untouched;
    EXPECT_EQ(bus.accept_all(&error), status::invalid_config);
    EXPECT_EQ(bus.backend().add_calls, 2);
    EXPECT_EQ(bus.backend().remove_calls, 1);
    EXPECT_EQ(bus.backend().last_removed, 4);
    EXPECT_EQ(error, 22); // rollback does not overwrite the error that caused it
}

// ---------------------------------------------------------------- remove / clear / status

TEST(CanBus, RemoveFilterForwardsHandle)
{
    probe_bus bus;
    int error = untouched;
    EXPECT_EQ(bus.remove_filter(9, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().last_removed, 9);

    bus.backend().result = status::not_found;
    EXPECT_EQ(bus.remove_filter(invalid_filter), status::not_found);
    EXPECT_EQ(bus.backend().last_removed, invalid_filter);
    EXPECT_EQ(bus.backend().remove_calls, 2);
}

TEST(CanBus, RemoveAndClearFiltersOverMock)
{
    basic_bus<mock::backend> bus;
    EXPECT_EQ(bus.remove_filter(0), status::not_configured);
    EXPECT_EQ(bus.clear_filters(), status::not_configured);

    ASSERT_EQ(bus.open("vcan0"), status::ok);
    filter_handle a = invalid_filter;
    filter_handle b = invalid_filter;
    ASSERT_EQ(bus.add_filter(filter::exact(1U), &a), status::ok);
    ASSERT_EQ(bus.add_filter(filter::exact(2U), &b), status::ok);

    int error = untouched;
    EXPECT_EQ(bus.remove_filter(a, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.remove_filter(a), status::not_found);
    EXPECT_EQ(bus.remove_filter(invalid_filter), status::not_found);
    EXPECT_EQ(bus.backend().inject(make_frame(1U)), status::not_found);
    EXPECT_EQ(bus.backend().inject(make_frame(2U)), status::ok);

    error = untouched;
    EXPECT_EQ(bus.clear_filters(&error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().filter_count(), 0U);
    EXPECT_EQ(bus.backend().inject(make_frame(2U)), status::not_found);
    EXPECT_EQ(bus.remove_filter(b), status::not_found);
}

TEST(CanBus, ClearFiltersForwards)
{
    probe_bus bus;
    int error = untouched;
    EXPECT_EQ(bus.clear_filters(&error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.backend().clear_calls, 1);
}

TEST(CanBus, QueryStatusForwardsAndFillsOutput)
{
    probe_bus bus;
    bus_status st;
    int error = untouched;
    ASSERT_EQ(bus.query_status(st, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(st.state, bus_state::error_passive);
    EXPECT_EQ(st.tx_error_counter, 11U);
    EXPECT_EQ(bus.backend().query_calls, 1);
}

TEST(CanBus, QueryStatusOverMock)
{
    basic_bus<mock::backend> bus;
    bus_status st;
    st.state = bus_state::bus_off;
    EXPECT_EQ(bus.query_status(st), status::not_configured);
    EXPECT_EQ(st.state, bus_state::stopped);

    ASSERT_EQ(bus.open("vcan0"), status::ok);
    bus.backend().set_bus_state(bus_state::error_warning, 100U, 2U);
    ASSERT_EQ(bus.query_status(st), status::ok);
    EXPECT_EQ(st.state, bus_state::error_warning);
    EXPECT_EQ(st.tx_error_counter, 100U);
    EXPECT_EQ(st.rx_error_counter, 2U);
}

// ---------------------------------------------------------------- system_error handling

TEST(CanBus, EveryCallClearsSystemErrorOnEntry)
{
    probe_bus bus; // set_errno stays -1: the backend never writes, only the facade clears
    int error;
    frame out;
    bus_status st;
    const uint8_t payload[1] = {1};

    error = untouched; EXPECT_EQ(bus.open(1, can_config(), &error), status::ok);                       EXPECT_EQ(error, 0);
    error = untouched; EXPECT_EQ(bus.close(&error), status::ok);                                       EXPECT_EQ(error, 0);
    error = untouched; EXPECT_EQ(bus.send(make_frame(1U), no_wait, &error), status::ok);               EXPECT_EQ(error, 0);
    error = untouched; EXPECT_EQ(bus.send(1U, payload, 1U, id_format::standard, no_wait, &error), status::ok);
    EXPECT_EQ(error, 0);
    error = untouched; EXPECT_EQ(bus.receive(out, no_wait, &error), status::ok);                       EXPECT_EQ(error, 0);
    error = untouched; EXPECT_EQ(bus.add_filter(filter::any(), nullptr, &error), status::ok);           EXPECT_EQ(error, 0);
    error = untouched; EXPECT_EQ(bus.accept_all(&error), status::ok);                                   EXPECT_EQ(error, 0);
    error = untouched; EXPECT_EQ(bus.remove_filter(0, &error), status::ok);                             EXPECT_EQ(error, 0);
    error = untouched; EXPECT_EQ(bus.clear_filters(&error), status::ok);                                EXPECT_EQ(error, 0);
    error = untouched; EXPECT_EQ(bus.query_status(st, &error), status::ok);                             EXPECT_EQ(error, 0);
}

TEST(CanBus, BackendSystemErrorReachesCaller)
{
    probe_bus bus;
    bus.backend().set_errno = 77;
    bus.backend().result = status::system_call_error;
    int error;
    frame out;
    bus_status st;
    const uint8_t payload[1] = {1};

    error = 0; EXPECT_EQ(bus.open(1, can_config(), &error), status::system_call_error);                EXPECT_EQ(error, 77);
    error = 0; EXPECT_EQ(bus.close(&error), status::system_call_error);                                EXPECT_EQ(error, 77);
    error = 0; EXPECT_EQ(bus.send(make_frame(1U), no_wait, &error), status::system_call_error);        EXPECT_EQ(error, 77);
    error = 0; EXPECT_EQ(bus.send(1U, payload, 1U, id_format::standard, no_wait, &error), status::system_call_error);
    EXPECT_EQ(error, 77);
    error = 0; EXPECT_EQ(bus.receive(out, no_wait, &error), status::system_call_error);                EXPECT_EQ(error, 77);
    error = 0; EXPECT_EQ(bus.add_filter(filter::any(), nullptr, &error), status::system_call_error);   EXPECT_EQ(error, 77);
    error = 0; EXPECT_EQ(bus.accept_all(&error), status::system_call_error);                           EXPECT_EQ(error, 77);
    error = 0; EXPECT_EQ(bus.remove_filter(0, &error), status::system_call_error);                     EXPECT_EQ(error, 77);
    error = 0; EXPECT_EQ(bus.clear_filters(&error), status::system_call_error);                        EXPECT_EQ(error, 77);
    error = 0; EXPECT_EQ(bus.query_status(st, &error), status::system_call_error);                     EXPECT_EQ(error, 77);
}

TEST(CanBus, NullSystemErrorIsAcceptedEverywhere)
{
    probe_bus bus;
    bus.backend().set_errno = 5;
    frame out;
    bus_status st;
    const uint8_t payload[1] = {1};
    EXPECT_EQ(bus.open(1), status::ok);
    EXPECT_EQ(bus.close(), status::ok);
    EXPECT_EQ(bus.send(make_frame(1U)), status::ok);
    EXPECT_EQ(bus.send(1U, payload, 1U), status::ok);
    EXPECT_EQ(bus.send(1U, payload, 9U), status::out_of_range);
    EXPECT_EQ(bus.receive(out), status::ok);
    EXPECT_EQ(bus.add_filter(filter::any()), status::ok);
    EXPECT_EQ(bus.accept_all(), status::ok);
    EXPECT_EQ(bus.remove_filter(0), status::ok);
    EXPECT_EQ(bus.clear_filters(), status::ok);
    EXPECT_EQ(bus.query_status(st), status::ok);
}

// ---------------------------------------------------------------- status passthrough

TEST(CanBus, BackendStatusCodesPassThroughUnchanged)
{
    const status codes[] = {
        status::ok,           status::full,         status::empty,       status::out_of_range,
        status::not_found,    status::not_configured, status::invalid_config, status::invalid_argument,
        status::invalid_callback, status::invalid_subscription, status::already_exists,
        status::system_call_error, status::data_loss, status::time_out,    status::unknown_error};

    for (const status code : codes)
    {
        probe_bus bus;
        bus.backend().result = code;
        frame out;
        bus_status st;
        filter_handle handle = 55;
        const uint8_t payload[2] = {1, 2};

        EXPECT_EQ(bus.open(1), code) << static_cast<int>(code);
        EXPECT_EQ(bus.close(), code) << static_cast<int>(code);
        EXPECT_EQ(bus.send(make_frame(1U)), code) << static_cast<int>(code);
        EXPECT_EQ(bus.send(1U, payload, 2U), code) << static_cast<int>(code);
        EXPECT_EQ(bus.receive(out), code) << static_cast<int>(code);
        EXPECT_EQ(bus.add_filter(filter::any(), &handle), code) << static_cast<int>(code);
        EXPECT_EQ(handle, code == status::ok ? bus.backend().handle_to_give : invalid_filter)
            << static_cast<int>(code);
        EXPECT_EQ(bus.accept_all(), code) << static_cast<int>(code);
        EXPECT_EQ(bus.remove_filter(3), code) << static_cast<int>(code);
        EXPECT_EQ(bus.clear_filters(), code) << static_cast<int>(code);
        EXPECT_EQ(bus.query_status(st), code) << static_cast<int>(code);

        // open, close, send x2, receive, add_filter, accept_all(first add fails or both succeed),
        // remove, clear, query: every operation reached the backend exactly once.
        EXPECT_EQ(bus.backend().open_calls, 1);
        EXPECT_EQ(bus.backend().close_calls, 1);
        EXPECT_EQ(bus.backend().send_calls, 2);
        EXPECT_EQ(bus.backend().receive_calls, 1);
        EXPECT_EQ(bus.backend().add_calls, code == status::ok ? 3 : 2);
        EXPECT_EQ(bus.backend().remove_calls, 1);
        EXPECT_EQ(bus.backend().clear_calls, 1);
        EXPECT_EQ(bus.backend().query_calls, 1);
    }
}

} // namespace
