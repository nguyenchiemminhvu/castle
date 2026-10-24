#include <gtest/gtest.h>

#include "castle_ext/io/can/backends/mock.hpp"
#include "castle_ext/io/can/bus.hpp"

#include <errno.h>
#include <limits.h>

namespace
{

using namespace castle::io::can;
using castle::status;

// Two receive slots, two transmit slots, two filters: every limit is one or two calls away.
using tiny_backend = mock::basic_backend<2U, 2U, 2U>;

frame make_frame(uint32_t id, id_format format = id_format::standard, uint8_t first_byte = 0U)
{
    frame value(id, format);
    const uint8_t payload[2] = {first_byte, static_cast<uint8_t>(first_byte + 1U)};
    EXPECT_EQ(value.set_data(payload, 2U), status::ok);
    return value;
}

template <typename Backend>
void open_backend(Backend& backend, can_mode mode = can_mode::normal)
{
    int error = 123;
    ASSERT_EQ(backend.open("vcan0", can_config(bitrate_500k, mode), &error), status::ok);
    ASSERT_EQ(error, 0);
}

template <typename Backend>
filter_handle install(Backend& backend, const filter& value)
{
    filter_handle handle = 77;
    EXPECT_EQ(backend.add_filter(value, handle, nullptr), status::ok);
    return handle;
}

// ---------------------------------------------------------------- open / close

TEST(CanMockBackend, StartsClosedWithDefaultState)
{
    mock::backend backend;
    EXPECT_FALSE(backend.is_open());
    EXPECT_EQ(backend.sent_count(), 0U);
    EXPECT_EQ(backend.filter_count(), 0U);
    EXPECT_EQ(backend.configuration().bitrate, 0U);
    EXPECT_EQ(backend.configuration().mode, can_mode::normal);
}

TEST(CanMockBackend, OpenRejectsNullAndEmptyEndpoint)
{
    mock::backend backend;
    int error = 123;

    EXPECT_EQ(backend.open(nullptr, can_config(), &error), status::invalid_argument);
    EXPECT_EQ(error, 0);
    EXPECT_FALSE(backend.is_open());

    error = 123;
    EXPECT_EQ(backend.open("", can_config(), &error), status::invalid_argument);
    EXPECT_EQ(error, 0);
    EXPECT_FALSE(backend.is_open());

    EXPECT_EQ(backend.open(nullptr, can_config(), nullptr), status::invalid_argument);
    EXPECT_EQ(backend.configuration().bitrate, 0U);
}

TEST(CanMockBackend, OpenRecordsConfigurationAndStartsErrorActive)
{
    mock::backend backend;
    int error = 123;
    ASSERT_EQ(backend.open("can0", can_config(bitrate_250k, can_mode::loopback), &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_TRUE(backend.is_open());
    EXPECT_EQ(backend.configuration().bitrate, bitrate_250k);
    EXPECT_EQ(backend.configuration().mode, can_mode::loopback);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.state, bus_state::error_active);
    EXPECT_EQ(st.tx_error_counter, 0U);
    EXPECT_EQ(st.rx_error_counter, 0U);
    EXPECT_EQ(st.rx_overruns, 0U);
    EXPECT_EQ(st.tx_failures, 0U);
}

TEST(CanMockBackend, SecondOpenFailsWithInvalidConfigAndKeepsExistingState)
{
    mock::backend backend;
    ASSERT_EQ(backend.open("can0", can_config(bitrate_250k, can_mode::normal), nullptr), status::ok);
    install(backend, filter::any());
    ASSERT_EQ(backend.send(make_frame(0x10U), no_wait, nullptr), status::ok);

    int error = 123;
    EXPECT_EQ(backend.open("can1", can_config(bitrate_1m, can_mode::loopback), &error), status::invalid_config);
    EXPECT_EQ(error, 0);

    EXPECT_TRUE(backend.is_open());
    EXPECT_EQ(backend.configuration().bitrate, bitrate_250k);
    EXPECT_EQ(backend.configuration().mode, can_mode::normal);
    EXPECT_EQ(backend.filter_count(), 1U);
    EXPECT_EQ(backend.sent_count(), 1U);
}

TEST(CanMockBackend, CloseTwiceReportsNotConfigured)
{
    mock::backend backend;
    open_backend(backend);

    int error = 123;
    EXPECT_EQ(backend.close(&error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_FALSE(backend.is_open());

    error = 123;
    EXPECT_EQ(backend.close(&error), status::not_configured);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.close(nullptr), status::not_configured);
}

TEST(CanMockBackend, EveryOperationOnClosedBackendReportsNotConfigured)
{
    mock::backend never_opened;
    mock::backend closed;
    open_backend(closed);
    ASSERT_EQ(closed.close(nullptr), status::ok);

    mock::backend* const backends[2] = {&never_opened, &closed};
    for (mock::backend* backend : backends)
    {
        int error = 123;
        frame out = make_frame(0x55U);
        EXPECT_EQ(backend->send(make_frame(1U), no_wait, &error), status::not_configured);
        EXPECT_EQ(error, 0);

        error = 123;
        EXPECT_EQ(backend->receive(out, no_wait, &error), status::not_configured);
        EXPECT_EQ(error, 0);
        EXPECT_EQ(out, make_frame(0x55U));

        error = 123;
        filter_handle handle = 5;
        EXPECT_EQ(backend->add_filter(filter::any(), handle, &error), status::not_configured);
        EXPECT_EQ(handle, invalid_filter);
        EXPECT_EQ(error, 0);

        error = 123;
        EXPECT_EQ(backend->remove_filter(0, &error), status::not_configured);
        EXPECT_EQ(error, 0);

        error = 123;
        EXPECT_EQ(backend->clear_filters(&error), status::not_configured);
        EXPECT_EQ(error, 0);

        error = 123;
        bus_status st;
        st.state = bus_state::bus_off;
        st.tx_error_counter = 9U;
        st.rx_error_counter = 8U;
        st.rx_overruns = 7U;
        st.tx_failures = 6U;
        EXPECT_EQ(backend->query_status(st, &error), status::not_configured);
        EXPECT_EQ(error, 0);
        EXPECT_EQ(st.state, bus_state::stopped);
        EXPECT_EQ(st.tx_error_counter, 0U);
        EXPECT_EQ(st.rx_error_counter, 0U);
        EXPECT_EQ(st.rx_overruns, 0U);
        EXPECT_EQ(st.tx_failures, 0U);

        EXPECT_EQ(backend->inject(make_frame(1U)), status::not_configured);
        EXPECT_EQ(backend->pop_sent(out), status::empty);
        EXPECT_EQ(backend->sent_count(), 0U);
        EXPECT_EQ(backend->filter_count(), 0U);
    }
}

TEST(CanMockBackend, CloseDiscardsFiltersAndQueues)
{
    tiny_backend backend;
    open_backend(backend);
    install(backend, filter::any());
    ASSERT_EQ(backend.inject(make_frame(1U)), status::ok);
    ASSERT_EQ(backend.send(make_frame(2U), no_wait, nullptr), status::ok);
    ASSERT_EQ(backend.close(nullptr), status::ok);

    EXPECT_EQ(backend.sent_count(), 0U);
    EXPECT_EQ(backend.filter_count(), 0U);
    frame out;
    EXPECT_EQ(backend.pop_sent(out), status::empty);

    bus_status st;
    EXPECT_EQ(backend.query_status(st, nullptr), status::not_configured);
    EXPECT_EQ(st.state, bus_state::stopped);
}

TEST(CanMockBackend, ReopenResetsCountersQueuesFiltersAndFaults)
{
    tiny_backend backend;
    open_backend(backend, can_mode::loopback);
    install(backend, filter::any());
    install(backend, filter::any(id_format::extended));
    // overflow rx (loopback delivers into rx), count an overrun
    for (uint32_t i = 0U; i < 3U; ++i)
    {
        ASSERT_EQ(backend.send(make_frame(i), no_wait, nullptr), status::ok);
    }
    backend.fail_next_send(status::unknown_error);
    ASSERT_EQ(backend.send(make_frame(9U), no_wait, nullptr), status::unknown_error);
    backend.fail_next_send(status::full); // still armed when the controller is reopened
    backend.set_bus_state(bus_state::error_passive, 130U, 7U);

    bus_status before;
    ASSERT_EQ(backend.query_status(before, nullptr), status::ok);
    EXPECT_EQ(before.rx_overruns, 1U);
    EXPECT_EQ(before.tx_failures, 1U);
    EXPECT_EQ(before.state, bus_state::error_passive);
    ASSERT_EQ(backend.close(nullptr), status::ok);

    ASSERT_EQ(backend.open("vcan1", can_config(bitrate_100k, can_mode::normal), nullptr), status::ok);
    bus_status after;
    ASSERT_EQ(backend.query_status(after, nullptr), status::ok);
    EXPECT_EQ(after.state, bus_state::error_active);
    EXPECT_EQ(after.tx_error_counter, 0U);
    EXPECT_EQ(after.rx_error_counter, 0U);
    EXPECT_EQ(after.rx_overruns, 0U);
    EXPECT_EQ(after.tx_failures, 0U);
    EXPECT_EQ(backend.filter_count(), 0U);
    EXPECT_EQ(backend.sent_count(), 0U);
    EXPECT_EQ(backend.configuration().bitrate, bitrate_100k);

    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, nullptr), status::ok); // fail_next_send was disarmed
    EXPECT_EQ(backend.sent_count(), 1U);
}

// ---------------------------------------------------------------- send

TEST(CanMockBackend, SentFramesPopInFifoOrder)
{
    mock::backend backend;
    open_backend(backend);

    for (uint32_t i = 0U; i < 5U; ++i)
    {
        int error = 123;
        ASSERT_EQ(backend.send(make_frame(0x100U + i, id_format::standard, static_cast<uint8_t>(i)), no_wait, &error),
                  status::ok);
        EXPECT_EQ(error, 0);
        EXPECT_EQ(backend.sent_count(), i + 1U);
    }

    for (uint32_t i = 0U; i < 5U; ++i)
    {
        frame out;
        ASSERT_EQ(backend.pop_sent(out), status::ok);
        EXPECT_EQ(out, make_frame(0x100U + i, id_format::standard, static_cast<uint8_t>(i)));
        EXPECT_EQ(backend.sent_count(), 4U - i);
    }
    frame out = make_frame(0x7FFU);
    EXPECT_EQ(backend.pop_sent(out), status::empty);
    EXPECT_EQ(out, make_frame(0x7FFU));
}

TEST(CanMockBackend, SendPreservesFormatTypeAndPayload)
{
    mock::backend backend;
    open_backend(backend);

    frame extended = make_frame(0x1ABCDE01U, id_format::extended, 0x40U);
    frame remote(0x321U);
    ASSERT_EQ(remote.set_remote(4U), status::ok);
    ASSERT_EQ(backend.send(extended, no_wait, nullptr), status::ok);
    ASSERT_EQ(backend.send(remote, no_wait, nullptr), status::ok);

    frame out;
    ASSERT_EQ(backend.pop_sent(out), status::ok);
    EXPECT_EQ(out, extended);
    EXPECT_TRUE(out.is_extended());
    ASSERT_EQ(backend.pop_sent(out), status::ok);
    EXPECT_EQ(out, remote);
    EXPECT_TRUE(out.is_remote());
    EXPECT_EQ(out.dlc, 4U);
}

TEST(CanMockBackend, TransmitQueueFullReportsFullUntilDrained)
{
    tiny_backend backend;
    open_backend(backend);

    EXPECT_EQ(backend.send(make_frame(1U), no_wait, nullptr), status::ok);
    EXPECT_EQ(backend.send(make_frame(2U), no_wait, nullptr), status::ok);
    EXPECT_EQ(backend.sent_count(), 2U);

    int error = 123;
    EXPECT_EQ(backend.send(make_frame(3U), no_wait, &error), status::full);
    EXPECT_EQ(error, 0);
    // The mock never blocks: a long timeout returns immediately with the same result.
    EXPECT_EQ(backend.send(make_frame(3U), wait_forever, nullptr), status::full);
    EXPECT_EQ(backend.sent_count(), 2U);

    frame out;
    ASSERT_EQ(backend.pop_sent(out), status::ok);
    EXPECT_EQ(out.id, 1U);
    EXPECT_EQ(backend.send(make_frame(3U), no_wait, nullptr), status::ok);
    ASSERT_EQ(backend.pop_sent(out), status::ok);
    EXPECT_EQ(out.id, 2U);
    ASSERT_EQ(backend.pop_sent(out), status::ok);
    EXPECT_EQ(out.id, 3U);
    EXPECT_EQ(backend.pop_sent(out), status::empty);
}

TEST(CanMockBackend, SendDoesNotReachOwnReceiveInNormalMode)
{
    mock::backend backend;
    open_backend(backend);
    install(backend, filter::any());
    ASSERT_EQ(backend.send(make_frame(0x10U), no_wait, nullptr), status::ok);

    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
    EXPECT_EQ(backend.sent_count(), 1U);
}

TEST(CanMockBackend, ListenOnlyRejectsSendButStillReceives)
{
    mock::backend backend;
    open_backend(backend, can_mode::listen_only);
    install(backend, filter::any());

    int error = 123;
    EXPECT_EQ(backend.send(make_frame(0x10U), no_wait, &error), status::invalid_config);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.sent_count(), 0U);

    ASSERT_EQ(backend.inject(make_frame(0x20U)), status::ok);
    frame out;
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x20U);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.tx_failures, 0U);
}

TEST(CanMockBackend, FailNextSendIsOneShotAndCountsTxFailure)
{
    mock::backend backend;
    open_backend(backend);

    backend.fail_next_send(status::unknown_error);
    int error = 123;
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, &error), status::unknown_error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.sent_count(), 0U);

    EXPECT_EQ(backend.send(make_frame(2U), no_wait, nullptr), status::ok);
    EXPECT_EQ(backend.sent_count(), 1U);

    backend.fail_next_send(status::full);
    EXPECT_EQ(backend.send(make_frame(3U), no_wait, nullptr), status::full);
    EXPECT_EQ(backend.send(make_frame(4U), no_wait, nullptr), status::ok);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.tx_failures, 2U);

    frame out;
    ASSERT_EQ(backend.pop_sent(out), status::ok);
    EXPECT_EQ(out.id, 2U);
    ASSERT_EQ(backend.pop_sent(out), status::ok);
    EXPECT_EQ(out.id, 4U);
    EXPECT_EQ(backend.pop_sent(out), status::empty);
}

TEST(CanMockBackend, FailNextSendOkDisarms)
{
    mock::backend backend;
    open_backend(backend);

    backend.fail_next_send(status::data_loss);
    backend.fail_next_send(status::ok);
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, nullptr), status::ok);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.tx_failures, 0U);
}

TEST(CanMockBackend, FailNextSendOverridesLoopbackDelivery)
{
    mock::backend backend;
    open_backend(backend, can_mode::loopback);
    install(backend, filter::any());

    backend.fail_next_send(status::system_call_error);
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, nullptr), status::system_call_error);
    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
}

TEST(CanMockBackend, BusOffSendFailsWithEnetunreach)
{
    mock::backend backend;
    open_backend(backend);
    backend.set_bus_state(bus_state::bus_off, 255U, 0U);

    int error = 0;
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, &error), status::system_call_error);
    EXPECT_EQ(error, ENETUNREACH);
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, nullptr), status::system_call_error);
    EXPECT_EQ(backend.sent_count(), 0U);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.state, bus_state::bus_off);
    EXPECT_EQ(st.tx_failures, 2U);

    // Recovering the bus makes transmission work again and the errno is cleared on entry.
    backend.set_bus_state(bus_state::error_active);
    error = 123;
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.sent_count(), 1U);
}

TEST(CanMockBackend, BusOffAlsoBlocksLoopbackAndArmedFailureWinsOverBusOff)
{
    mock::backend backend;
    open_backend(backend, can_mode::loopback);
    install(backend, filter::any());
    backend.set_bus_state(bus_state::bus_off);

    int error = 0;
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, &error), status::system_call_error);
    EXPECT_EQ(error, ENETUNREACH);
    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);

    backend.fail_next_send(status::full);
    error = 123;
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, &error), status::full);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, &error), status::system_call_error);
    EXPECT_EQ(error, ENETUNREACH);
}

// ---------------------------------------------------------------- loopback

TEST(CanMockBackend, LoopbackDeliversToOwnReceiveOnlyThroughMatchingFilter)
{
    mock::backend backend;
    open_backend(backend, can_mode::loopback);

    // No filter installed: the frame is sent successfully but nothing is received.
    ASSERT_EQ(backend.send(make_frame(0x100U), no_wait, nullptr), status::ok);
    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);

    // Filter that does not match.
    install(backend, filter::exact(0x200U));
    ASSERT_EQ(backend.send(make_frame(0x100U), no_wait, nullptr), status::ok);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);

    // Wrong format never matches.
    ASSERT_EQ(backend.send(make_frame(0x200U, id_format::extended), no_wait, nullptr), status::ok);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);

    // Matching filter delivers.
    frame wanted = make_frame(0x200U, id_format::standard, 0x33U);
    ASSERT_EQ(backend.send(wanted, no_wait, nullptr), status::ok);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out, wanted);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);

    // Loopback never uses the transmit queue.
    EXPECT_EQ(backend.sent_count(), 0U);
    EXPECT_EQ(backend.pop_sent(out), status::empty);
}

TEST(CanMockBackend, LoopbackIgnoresTransmitQueueCapacity)
{
    tiny_backend backend;
    open_backend(backend, can_mode::loopback);
    install(backend, filter::any());

    // More sends than TxQueueDepth succeed, the receive queue is what fills up.
    EXPECT_EQ(backend.send(make_frame(1U), no_wait, nullptr), status::ok);
    EXPECT_EQ(backend.send(make_frame(2U), no_wait, nullptr), status::ok);
    EXPECT_EQ(backend.send(make_frame(3U), no_wait, nullptr), status::ok);
    EXPECT_EQ(backend.sent_count(), 0U);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.rx_overruns, 1U);

    frame out;
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 1U);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 2U);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
}

// ---------------------------------------------------------------- inject / receive

TEST(CanMockBackend, InjectWithoutFilterIsNotFound)
{
    mock::backend backend;
    open_backend(backend);

    EXPECT_EQ(backend.inject(make_frame(1U)), status::not_found);
    frame out;
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.rx_overruns, 0U);
}

TEST(CanMockBackend, InjectHonoursFormatAndMask)
{
    mock::backend backend;
    open_backend(backend);
    install(backend, filter(0x120U, 0x7F0U)); // 0x120..0x12F standard

    EXPECT_EQ(backend.inject(make_frame(0x120U)), status::ok);
    EXPECT_EQ(backend.inject(make_frame(0x12FU)), status::ok);
    EXPECT_EQ(backend.inject(make_frame(0x130U)), status::not_found);
    EXPECT_EQ(backend.inject(make_frame(0x11FU)), status::not_found);
    EXPECT_EQ(backend.inject(make_frame(0x120U, id_format::extended)), status::not_found);

    frame remote(0x121U);
    ASSERT_EQ(remote.set_remote(2U), status::ok);
    EXPECT_EQ(backend.inject(remote), status::ok); // remote frames are not distinguished by filters

    frame out;
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x120U);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 0x12FU);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out, remote);
    EXPECT_EQ(backend.receive(out, no_wait, nullptr), status::empty);
}

TEST(CanMockBackend, InjectAcceptedByAnyOfSeveralFilters)
{
    mock::backend backend;
    open_backend(backend);
    install(backend, filter::exact(0x10U));
    install(backend, filter::exact(0x1FFFFFFFU, id_format::extended));

    EXPECT_EQ(backend.inject(make_frame(0x10U)), status::ok);
    EXPECT_EQ(backend.inject(make_frame(0x1FFFFFFFU, id_format::extended)), status::ok);
    EXPECT_EQ(backend.inject(make_frame(0x11U)), status::not_found);
    EXPECT_EQ(backend.inject(make_frame(0x10U, id_format::extended)), status::not_found);
}

TEST(CanMockBackend, InjectOverrunReportsFullAndCountsOverruns)
{
    tiny_backend backend;
    open_backend(backend);
    install(backend, filter::any());

    EXPECT_EQ(backend.inject(make_frame(1U)), status::ok);
    EXPECT_EQ(backend.inject(make_frame(2U)), status::ok);
    EXPECT_EQ(backend.inject(make_frame(3U)), status::full);
    EXPECT_EQ(backend.inject(make_frame(4U)), status::full);
    // A frame that no filter accepts is not an overrun.
    EXPECT_EQ(backend.inject(make_frame(5U, id_format::extended)), status::not_found);

    bus_status st;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.rx_overruns, 2U);

    // The oldest frames survive; draining makes room again.
    frame out;
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 1U);
    EXPECT_EQ(backend.inject(make_frame(6U)), status::ok);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 2U);
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out.id, 6U);
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.rx_overruns, 2U);
}

TEST(CanMockBackend, ReceiveIsFifoAndEmptyLeavesOutputUntouched)
{
    mock::backend backend;
    open_backend(backend);
    install(backend, filter::any());
    install(backend, filter::any(id_format::extended));

    ASSERT_EQ(backend.inject(make_frame(0x300U, id_format::standard, 1U)), status::ok);
    ASSERT_EQ(backend.inject(make_frame(0x400U, id_format::extended, 2U)), status::ok);
    ASSERT_EQ(backend.inject(make_frame(0x100U, id_format::standard, 3U)), status::ok);

    int error = 123;
    frame out;
    ASSERT_EQ(backend.receive(out, no_wait, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(out, make_frame(0x300U, id_format::standard, 1U));
    ASSERT_EQ(backend.receive(out, wait_forever, nullptr), status::ok);
    EXPECT_EQ(out, make_frame(0x400U, id_format::extended, 2U));
    ASSERT_EQ(backend.receive(out, no_wait, nullptr), status::ok);
    EXPECT_EQ(out, make_frame(0x100U, id_format::standard, 3U));

    const frame sentinel = make_frame(0x555U, id_format::standard, 9U);
    out = sentinel;
    error = 123;
    EXPECT_EQ(backend.receive(out, no_wait, &error), status::empty);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(out, sentinel);
    // The mock never blocks, even for an unbounded wait.
    EXPECT_EQ(backend.receive(out, wait_forever, nullptr), status::empty);
}

// ---------------------------------------------------------------- filters

TEST(CanMockBackend, AddFilterAssignsLowestFreeHandlesUntilFull)
{
    tiny_backend backend;
    open_backend(backend);

    filter_handle first = 77;
    filter_handle second = 77;
    filter_handle third = 77;
    int error = 123;
    EXPECT_EQ(backend.add_filter(filter::exact(1U), first, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.add_filter(filter::exact(2U), second, nullptr), status::ok);
    EXPECT_EQ(first, 0);
    EXPECT_EQ(second, 1);
    EXPECT_EQ(backend.filter_count(), 2U);

    error = 123;
    EXPECT_EQ(backend.add_filter(filter::exact(3U), third, &error), status::full);
    EXPECT_EQ(third, invalid_filter);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.filter_count(), 2U);
    EXPECT_EQ(backend.inject(make_frame(3U)), status::not_found);
}

TEST(CanMockBackend, FilterHandleIsReusedAfterRemove)
{
    tiny_backend backend;
    open_backend(backend);

    const filter_handle a = install(backend, filter::exact(1U));
    const filter_handle b = install(backend, filter::exact(2U));
    ASSERT_EQ(a, 0);
    ASSERT_EQ(b, 1);

    int error = 123;
    ASSERT_EQ(backend.remove_filter(a, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.filter_count(), 1U);
    EXPECT_EQ(backend.inject(make_frame(1U)), status::not_found);
    EXPECT_EQ(backend.inject(make_frame(2U)), status::ok);

    const filter_handle c = install(backend, filter::exact(3U));
    EXPECT_EQ(c, a);
    EXPECT_EQ(backend.inject(make_frame(3U)), status::ok);
    EXPECT_EQ(backend.inject(make_frame(1U)), status::not_found);

    // Free the higher slot; the next filter takes it again.
    ASSERT_EQ(backend.remove_filter(b, nullptr), status::ok);
    EXPECT_EQ(install(backend, filter::exact(4U)), b);
    EXPECT_EQ(backend.filter_count(), 2U);
}

TEST(CanMockBackend, RemoveFilterRejectsStaleAndInvalidHandles)
{
    tiny_backend backend;
    open_backend(backend);
    const filter_handle handle = install(backend, filter::any());

    int error = 123;
    EXPECT_EQ(backend.remove_filter(1, &error), status::not_found); // never used slot
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.remove_filter(invalid_filter, nullptr), status::not_found);
    EXPECT_EQ(backend.remove_filter(-5, nullptr), status::not_found);
    EXPECT_EQ(backend.remove_filter(2, nullptr), status::not_found); // == MaxFilters
    EXPECT_EQ(backend.remove_filter(INT_MAX, nullptr), status::not_found);
    EXPECT_EQ(backend.filter_count(), 1U);

    ASSERT_EQ(backend.remove_filter(handle, nullptr), status::ok);
    EXPECT_EQ(backend.remove_filter(handle, nullptr), status::not_found); // stale
    EXPECT_EQ(backend.filter_count(), 0U);
}

TEST(CanMockBackend, ClearFiltersRemovesEverything)
{
    tiny_backend backend;
    open_backend(backend);
    install(backend, filter::any());
    install(backend, filter::any(id_format::extended));
    ASSERT_EQ(backend.filter_count(), 2U);

    int error = 123;
    EXPECT_EQ(backend.clear_filters(&error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(backend.filter_count(), 0U);
    EXPECT_EQ(backend.inject(make_frame(1U)), status::not_found);
    EXPECT_EQ(backend.inject(make_frame(1U, id_format::extended)), status::not_found);

    EXPECT_EQ(backend.clear_filters(nullptr), status::ok); // idempotent
    EXPECT_EQ(install(backend, filter::any()), 0);         // all slots free again
    EXPECT_EQ(install(backend, filter::any()), 1);
}

// ---------------------------------------------------------------- state and status

TEST(CanMockBackend, SetBusStateIsReportedByQueryStatus)
{
    mock::backend backend;
    open_backend(backend);

    backend.set_bus_state(bus_state::error_passive, 130U, 7U);
    bus_status st;
    int error = 123;
    ASSERT_EQ(backend.query_status(st, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(st.state, bus_state::error_passive);
    EXPECT_EQ(st.tx_error_counter, 130U);
    EXPECT_EQ(st.rx_error_counter, 7U);

    backend.set_bus_state(bus_state::error_warning);
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.state, bus_state::error_warning);
    EXPECT_EQ(st.tx_error_counter, 0U);
    EXPECT_EQ(st.rx_error_counter, 0U);
    EXPECT_TRUE(is_operational(st.state));

    backend.set_bus_state(bus_state::bus_off, 255U, 255U);
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.state, bus_state::bus_off);
    EXPECT_EQ(st.tx_error_counter, 255U);
    EXPECT_EQ(st.rx_error_counter, 255U);
    EXPECT_FALSE(is_operational(st.state));
}

TEST(CanMockBackend, QueryStatusOverwritesAllOutputFields)
{
    mock::backend backend;
    open_backend(backend);

    bus_status st;
    st.state = bus_state::bus_off;
    st.tx_error_counter = 200U;
    st.rx_error_counter = 201U;
    st.rx_overruns = 202U;
    st.tx_failures = 203U;
    ASSERT_EQ(backend.query_status(st, nullptr), status::ok);
    EXPECT_EQ(st.state, bus_state::error_active);
    EXPECT_EQ(st.tx_error_counter, 0U);
    EXPECT_EQ(st.rx_error_counter, 0U);
    EXPECT_EQ(st.rx_overruns, 0U);
    EXPECT_EQ(st.tx_failures, 0U);
}

TEST(CanMockBackend, SystemErrorIsClearedByEveryOperationWhenOpen)
{
    tiny_backend backend;
    open_backend(backend);
    int error = 123;
    frame out;
    filter_handle handle = invalid_filter;
    bus_status st;

    error = 123;
    EXPECT_EQ(backend.add_filter(filter::any(), handle, &error), status::ok);
    EXPECT_EQ(error, 0);
    error = 123;
    EXPECT_EQ(backend.receive(out, no_wait, &error), status::empty);
    EXPECT_EQ(error, 0);
    error = 123;
    EXPECT_EQ(backend.remove_filter(handle, &error), status::ok);
    EXPECT_EQ(error, 0);
    error = 123;
    EXPECT_EQ(backend.clear_filters(&error), status::ok);
    EXPECT_EQ(error, 0);
    error = 123;
    EXPECT_EQ(backend.query_status(st, &error), status::ok);
    EXPECT_EQ(error, 0);
}

// ---------------------------------------------------------------- through basic_bus

TEST(CanMockBackend, BusFacadeOpenValidatesEndpointThroughBackend)
{
    basic_bus<mock::backend> bus;
    int error = 123;
    EXPECT_EQ(bus.open(nullptr, can_config(), &error), status::invalid_argument);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(bus.open("", can_config(), nullptr), status::invalid_argument);
    EXPECT_FALSE(bus.is_open());

    ASSERT_EQ(bus.open("vcan0", can_config(bitrate_125k, can_mode::loopback), nullptr), status::ok);
    EXPECT_TRUE(bus.is_open());
    EXPECT_EQ(bus.backend().configuration().bitrate, bitrate_125k);
    EXPECT_EQ(bus.backend().configuration().mode, can_mode::loopback);
    EXPECT_EQ(bus.open("vcan0", can_config(), nullptr), status::invalid_config);

    EXPECT_EQ(bus.close(nullptr), status::ok);
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.close(nullptr), status::not_configured);
}

TEST(CanMockBackend, BusFacadeSendQueuesAndNormalModeNeverLoopsBack)
{
    basic_bus<tiny_backend> bus;
    ASSERT_EQ(bus.open("vcan0", can_config(bitrate_500k), nullptr), status::ok);
    ASSERT_EQ(bus.accept_all(), status::ok);

    const uint8_t payload[3] = {0xDEU, 0xADU, 0xBEU};
    ASSERT_EQ(bus.send(0x123U, payload, 3U), status::ok);
    ASSERT_EQ(bus.send(0x1ABCDEFU, nullptr, 0U, id_format::extended), status::ok);
    EXPECT_EQ(bus.send(0x7FFU, payload, 3U, id_format::standard, no_wait), status::full);
    EXPECT_EQ(bus.backend().sent_count(), 2U);

    frame out;
    EXPECT_EQ(bus.receive(out), status::empty);

    ASSERT_EQ(bus.backend().pop_sent(out), status::ok);
    EXPECT_EQ(out.id, 0x123U);
    EXPECT_FALSE(out.is_extended());
    EXPECT_EQ(out.dlc, 3U);
    EXPECT_EQ(out.data[0], 0xDEU);
    EXPECT_EQ(out.data[2], 0xBEU);
    ASSERT_EQ(bus.backend().pop_sent(out), status::ok);
    EXPECT_EQ(out.id, 0x1ABCDEFU);
    EXPECT_TRUE(out.is_extended());
    EXPECT_EQ(out.dlc, 0U);
}

TEST(CanMockBackend, BusFacadeLoopbackRoundTrip)
{
    basic_bus<mock::backend> bus;
    ASSERT_EQ(bus.open("vcan0", can_config(bitrate_500k, can_mode::loopback), nullptr), status::ok);
    ASSERT_EQ(bus.accept_all(), status::ok);

    const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_EQ(bus.send(0x7FFU, payload, 8U), status::ok);
    ASSERT_EQ(bus.send(0x1FFFFFFFU, payload, 4U, id_format::extended), status::ok);

    frame standard;
    frame extended;
    ASSERT_EQ(bus.receive(standard, castle::chrono::milliseconds(10)), status::ok);
    ASSERT_EQ(bus.receive(extended, castle::chrono::milliseconds(10)), status::ok);
    EXPECT_EQ(standard.id, 0x7FFU);
    EXPECT_EQ(standard.dlc, 8U);
    EXPECT_EQ(standard.data[7], 8U);
    EXPECT_EQ(extended.id, 0x1FFFFFFFU);
    EXPECT_TRUE(extended.is_extended());
    EXPECT_EQ(extended.dlc, 4U);
    EXPECT_EQ(bus.backend().sent_count(), 0U);
    EXPECT_EQ(bus.receive(standard), status::empty);
}

TEST(CanMockBackend, BusFacadePropagatesBusOffErrnoAndStatusCounters)
{
    basic_bus<mock::backend> bus;
    ASSERT_EQ(bus.open("vcan0", can_config(), nullptr), status::ok);
    bus.backend().set_bus_state(bus_state::bus_off, 255U, 3U);

    int error = 123;
    const uint8_t payload[1] = {0x5AU};
    EXPECT_EQ(bus.send(0x10U, payload, 1U, id_format::standard, no_wait, &error), status::system_call_error);
    EXPECT_EQ(error, ENETUNREACH);

    bus_status st;
    error = 123;
    ASSERT_EQ(bus.query_status(st, &error), status::ok);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(st.state, bus_state::bus_off);
    EXPECT_EQ(st.tx_error_counter, 255U);
    EXPECT_EQ(st.rx_error_counter, 3U);
    EXPECT_EQ(st.tx_failures, 1U);
}

TEST(CanMockBackend, BusFacadeInjectedFramesFollowInstalledFilters)
{
    basic_bus<mock::backend> bus;
    EXPECT_EQ(bus.backend().inject(make_frame(1U)), status::not_configured);
    ASSERT_EQ(bus.open("vcan0", can_config(), nullptr), status::ok);
    EXPECT_EQ(bus.backend().inject(make_frame(1U)), status::not_found);

    filter_handle handle = invalid_filter;
    ASSERT_EQ(bus.add_filter(filter::exact(0x42U), &handle), status::ok);
    EXPECT_EQ(bus.backend().inject(make_frame(0x42U)), status::ok);
    EXPECT_EQ(bus.backend().inject(make_frame(0x43U)), status::not_found);

    frame out;
    ASSERT_EQ(bus.receive(out), status::ok);
    EXPECT_EQ(out.id, 0x42U);

    ASSERT_EQ(bus.remove_filter(handle), status::ok);
    EXPECT_EQ(bus.backend().inject(make_frame(0x42U)), status::not_found);
    EXPECT_EQ(bus.remove_filter(handle), status::not_found);
    EXPECT_EQ(bus.receive(out), status::empty);
}

} // namespace
