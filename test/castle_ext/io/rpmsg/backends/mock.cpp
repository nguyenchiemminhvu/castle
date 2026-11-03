#include <gtest/gtest.h>
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

using namespace castle::io::rpmsg;

namespace
{
using small_mock = mock::basic_backend<2U, 16U>;

castle::status accept_message(void*, message_view const&) { return castle::status::ok; }
void ignore_unbind(void*) {}
void count_unbind(void* context) { ++*static_cast<uint32_t*>(context); }
void count_name_service(void* context, name_service_event const&) { ++*static_cast<uint32_t*>(context); }
} // namespace

TEST(RpmsgMockBackend, InjectAnnouncementBindAndUnbind)
{
    mock::backend backend;
    device<mock::backend> dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    uint32_t received = 0U;
    uint32_t unbound = 0U;
    endpoint<mock::backend> ept;
    ASSERT_EQ(ept.create(dev, "mock.service", rpmsg_addr_any, rpmsg_addr_any,
        [&received](message_view const& message) { received += static_cast<uint32_t>(message.size); },
        [&unbound]() { ++unbound; }), castle::status::ok);
    ASSERT_EQ(backend.announce("mock.service", 9900U), castle::status::ok);
    EXPECT_EQ(ept.destination_address(), 9900U);
    const uint8_t bytes[] = {5U, 6U};
    EXPECT_EQ(backend.inject(ept.source_address(), 9900U, bytes, sizeof(bytes)), castle::status::ok);
    EXPECT_EQ(received, 2U);
    EXPECT_EQ(backend.inject(1U, 9900U, bytes, sizeof(bytes)), castle::status::not_found);
    EXPECT_EQ(backend.inject(ept.source_address(), 9900U, nullptr, 2U), castle::status::invalid_argument);
    EXPECT_EQ(backend.unbind(ept.source_address()), castle::status::ok);
    EXPECT_EQ(unbound, 1U);
    EXPECT_EQ(backend.unbind(ept.source_address()), castle::status::not_found);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, FailureInjectionAndTxBufferExhaustion)
{
    mock::basic_backend<1U, 8U> backend;
    device<mock::basic_backend<1U, 8U>, options<1U, 1U, 12U, 8U, 2U, 24U> > dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    endpoint<mock::basic_backend<1U, 8U>, options<1U, 1U, 12U, 8U, 2U, 24U> > ept;
    ASSERT_EQ(ept.create(dev, "bounded", rpmsg_addr_any, 1234U, [](message_view const&) {}), castle::status::ok);
    const uint8_t payload[] = {1U};
    backend.fail_next_send(castle::status::system_call_error);
    EXPECT_EQ(ept.try_send(payload, sizeof(payload)), castle::status::system_call_error);
    EXPECT_EQ(ept.try_send(payload, sizeof(payload)), castle::status::ok);
    tx_buffer first;
    tx_buffer second;
    ASSERT_EQ(ept.acquire_tx_buffer(first), castle::status::ok);
    EXPECT_EQ(ept.acquire_tx_buffer(second), castle::status::full);
    EXPECT_EQ(ept.release_tx_buffer(first), castle::status::ok);
    EXPECT_EQ(backend.announce(nullptr, 1000U), castle::status::invalid_argument);
    EXPECT_EQ(backend.announce("", 1000U), castle::status::invalid_argument);
    EXPECT_EQ(backend.announce("name", rpmsg_addr_any), castle::status::invalid_argument);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, UninitializedOperationsReportNotConfigured)
{
    mock::backend backend;
    void* handle = nullptr;
    address actual = rpmsg_addr_any;
    void* data = nullptr;
    castle::size_type capacity = 0U;
    const uint8_t byte = 1U;
    EXPECT_FALSE(backend.is_initialized());
    EXPECT_EQ(backend.shutdown(), castle::status::not_configured);
    EXPECT_EQ(backend.create_endpoint("svc", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual),
              castle::status::not_configured);
    EXPECT_EQ(backend.send(nullptr, 1024U, 2000U, &byte, 1U, no_wait), castle::status::not_configured);
    EXPECT_EQ(backend.acquire_tx_buffer(nullptr, false, data, capacity), castle::status::not_configured);
    EXPECT_EQ(backend.poll(), castle::status::not_configured);
    EXPECT_EQ(backend.inject(1024U, 2000U, &byte, 1U), castle::status::not_configured);
    EXPECT_EQ(backend.announce("svc", 2000U), castle::status::not_configured);
    EXPECT_EQ(backend.bind(1024U, 2000U), castle::status::not_configured);
    EXPECT_EQ(backend.unbind(1024U), castle::status::not_configured);
    EXPECT_EQ(backend.remote_address(nullptr), rpmsg_addr_any);
}

TEST(RpmsgMockBackend, InitializeAndShutdownAreNotRepeatable)
{
    small_mock backend;
    device_config config;
    ASSERT_EQ(backend.initialize(config, nullptr, nullptr), castle::status::ok);
    EXPECT_TRUE(backend.is_initialized());
    EXPECT_EQ(backend.initialize(config, nullptr, nullptr), castle::status::invalid_config);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
    EXPECT_FALSE(backend.is_initialized());
    EXPECT_EQ(backend.shutdown(), castle::status::not_configured);
}

TEST(RpmsgMockBackend, CreateEndpointRejectsInvalidArguments)
{
    small_mock backend;
    ASSERT_EQ(backend.initialize(device_config(), nullptr, nullptr), castle::status::ok);
    void* handle = &backend;
    address actual = 0U;
    EXPECT_EQ(backend.create_endpoint(nullptr, 1024U, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual),
              castle::status::invalid_argument);
    EXPECT_EQ(handle, nullptr);
    EXPECT_EQ(actual, rpmsg_addr_any);
    EXPECT_EQ(backend.create_endpoint("", 1024U, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual),
              castle::status::invalid_argument);
    EXPECT_EQ(backend.create_endpoint("svc", 1024U, rpmsg_addr_any, nullptr,
                                      nullptr, &ignore_unbind, handle, actual),
              castle::status::invalid_argument);
    EXPECT_EQ(backend.create_endpoint("svc", 5U, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual),
              castle::status::invalid_argument);
    EXPECT_EQ(backend.create_endpoint("svc", 1024U, 35U, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual),
              castle::status::invalid_argument);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, CreateEndpointNameLengthAndTableCapacity)
{
    small_mock backend;
    ASSERT_EQ(backend.initialize(device_config(), nullptr, nullptr), castle::status::ok);
    void* first = nullptr;
    void* second = nullptr;
    void* third = nullptr;
    address a = 0U;
    address b = 0U;
    address c = 0U;
    EXPECT_EQ(backend.create_endpoint("abcdefghijklmnopqrstuvwxyz012345", rpmsg_addr_any, rpmsg_addr_any,
                                      nullptr, &accept_message, &ignore_unbind, third, c),
              castle::status::out_of_range);
    EXPECT_EQ(backend.create_endpoint("abcdefghijklmnopqrstuvwxyz0123456789", rpmsg_addr_any, rpmsg_addr_any,
                                      nullptr, &accept_message, &ignore_unbind, third, c),
              castle::status::out_of_range);
    ASSERT_EQ(backend.create_endpoint("first", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, first, a), castle::status::ok);
    ASSERT_EQ(backend.create_endpoint("second", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, second, b), castle::status::ok);
    EXPECT_EQ(backend.create_endpoint("third", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, third, c),
              castle::status::full);
    EXPECT_EQ(third, nullptr);
    backend.destroy_endpoint(first);
    ASSERT_EQ(backend.create_endpoint("third", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, third, c), castle::status::ok);
    EXPECT_NE(third, nullptr);
    EXPECT_NE(c, rpmsg_addr_any);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, DestroyAndSendIgnoreUnknownHandles)
{
    small_mock backend;
    ASSERT_EQ(backend.initialize(device_config(), nullptr, nullptr), castle::status::ok);
    void* handle = nullptr;
    address actual = 0U;
    ASSERT_EQ(backend.create_endpoint("svc", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual), castle::status::ok);
    int unrelated = 0;
    const uint8_t byte = 1U;
    backend.destroy_endpoint(nullptr);
    backend.destroy_endpoint(&unrelated);
    EXPECT_EQ(backend.remote_address(&unrelated), rpmsg_addr_any);
    EXPECT_EQ(backend.send(&unrelated, actual, 2000U, &byte, 1U, no_wait), castle::status::not_configured);
    EXPECT_EQ(backend.send(handle, actual, 2000U, &byte, 1U, no_wait), castle::status::ok);
    backend.destroy_endpoint(handle);
    EXPECT_EQ(backend.send(handle, actual, 2000U, &byte, 1U, no_wait), castle::status::not_configured);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, SendValidatesArgumentsAndPayloadLimit)
{
    small_mock backend;
    ASSERT_EQ(backend.initialize(device_config(), nullptr, nullptr), castle::status::ok);
    void* handle = nullptr;
    address actual = 0U;
    ASSERT_EQ(backend.create_endpoint("tx", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual), castle::status::ok);
    const uint8_t bytes[16] = {};
    EXPECT_EQ(backend.send(handle, 5U, 2000U, bytes, 1U, no_wait), castle::status::invalid_argument);
    EXPECT_EQ(backend.send(handle, actual, 2000U, nullptr, 1U, no_wait), castle::status::invalid_argument);
    EXPECT_EQ(backend.send(handle, actual, rpmsg_addr_any, bytes, 1U, no_wait), castle::status::invalid_argument);
    EXPECT_EQ(backend.send(handle, actual, 35U, bytes, 1U, no_wait), castle::status::invalid_argument);
    EXPECT_EQ(backend.send(handle, actual, 2000U, bytes, 17U, no_wait), castle::status::out_of_range);
    EXPECT_EQ(backend.send_count(), 0U);
    EXPECT_EQ(backend.send(handle, actual, 2000U, bytes, 16U, no_wait), castle::status::ok);
    EXPECT_EQ(backend.send_count(), 1U);
    EXPECT_EQ(backend.last_packet().size, 16U);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, TxBufferProtocolRejectsStaleAndForeignBuffers)
{
    small_mock backend;
    ASSERT_EQ(backend.initialize(device_config(), nullptr, nullptr), castle::status::ok);
    void* handle = nullptr;
    address actual = 0U;
    ASSERT_EQ(backend.create_endpoint("tx", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual), castle::status::ok);
    void* data = nullptr;
    castle::size_type capacity = 0U;
    uint8_t foreign[4] = {};
    EXPECT_EQ(backend.acquire_tx_buffer(nullptr, false, data, capacity), castle::status::not_configured);
    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(capacity, 0U);
    EXPECT_EQ(backend.send_tx_buffer(nullptr, actual, 2000U, foreign, 1U), castle::status::invalid_argument);
    EXPECT_EQ(backend.release_tx_buffer(nullptr, foreign), castle::status::invalid_argument);
    EXPECT_EQ(backend.send_tx_buffer(handle, actual, 2000U, foreign, 1U), castle::status::invalid_argument);
    EXPECT_EQ(backend.release_tx_buffer(handle, foreign), castle::status::invalid_argument);
    ASSERT_EQ(backend.acquire_tx_buffer(handle, false, data, capacity), castle::status::ok);
    EXPECT_EQ(capacity, 16U);
    void* second = nullptr;
    castle::size_type second_capacity = 0U;
    EXPECT_EQ(backend.acquire_tx_buffer(handle, false, second, second_capacity), castle::status::full);
    EXPECT_EQ(second, nullptr);
    EXPECT_EQ(backend.send_tx_buffer(handle, actual, 2000U, foreign, 1U), castle::status::invalid_argument);
    EXPECT_EQ(backend.release_tx_buffer(handle, foreign), castle::status::invalid_argument);
    backend.fail_next_send(castle::status::system_call_error);
    EXPECT_EQ(backend.send_tx_buffer(handle, actual, 2000U, data, 1U), castle::status::system_call_error);
    EXPECT_EQ(backend.release_tx_buffer(handle, data), castle::status::ok);
    EXPECT_EQ(backend.send_tx_buffer(handle, actual, 2000U, data, 1U), castle::status::invalid_argument);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, PollRemoteAddressAndInjectValidation)
{
    small_mock backend;
    EXPECT_EQ(backend.poll(), castle::status::not_configured);
    ASSERT_EQ(backend.initialize(device_config(), nullptr, nullptr), castle::status::ok);
    EXPECT_EQ(backend.poll(), castle::status::ok);
    EXPECT_EQ(backend.remote_address(nullptr), rpmsg_addr_any);
    void* handle = nullptr;
    address actual = 0U;
    ASSERT_EQ(backend.create_endpoint("rx", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual), castle::status::ok);
    EXPECT_EQ(backend.remote_address(handle), rpmsg_addr_any);
    const uint8_t byte = 1U;
    EXPECT_EQ(backend.inject(actual, 2000U, nullptr, 1U), castle::status::invalid_argument);
    EXPECT_EQ(backend.inject(actual, 2000U, &byte, 17U), castle::status::invalid_argument);
    EXPECT_EQ(backend.inject(999U, 2000U, &byte, 1U), castle::status::not_found);
    EXPECT_EQ(backend.inject(actual, 2000U, &byte, 1U), castle::status::ok);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, AnnounceValidatesNameAndRemote)
{
    small_mock backend;
    EXPECT_EQ(backend.announce("svc", 2000U), castle::status::not_configured);
    ASSERT_EQ(backend.initialize(device_config(), nullptr, nullptr), castle::status::ok);
    EXPECT_EQ(backend.announce(nullptr, 2000U), castle::status::invalid_argument);
    EXPECT_EQ(backend.announce("", 2000U), castle::status::invalid_argument);
    EXPECT_EQ(backend.announce("svc", rpmsg_addr_any), castle::status::invalid_argument);
    EXPECT_EQ(backend.announce("abcdefghijklmnopqrstuvwxyz0123456789", 2000U), castle::status::out_of_range);
    EXPECT_EQ(backend.announce("svc", 2000U), castle::status::not_configured);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, AnnounceMatchesBoundRemoteOrNotifiesNameService)
{
    small_mock backend;
    uint32_t announcements = 0U;
    ASSERT_EQ(backend.initialize(device_config(), &announcements, &count_name_service), castle::status::ok);
    void* handle = nullptr;
    address actual = 0U;
    ASSERT_EQ(backend.create_endpoint("svc", rpmsg_addr_any, 2000U, nullptr,
                                      &accept_message, &ignore_unbind, handle, actual), castle::status::ok);
    EXPECT_EQ(backend.announce("svc", 2000U), castle::status::ok);
    EXPECT_EQ(announcements, 0U);
    EXPECT_EQ(backend.announce("svc", 2100U), castle::status::ok);
    EXPECT_EQ(announcements, 1U);
    EXPECT_EQ(backend.announce("orphan", 2200U), castle::status::ok);
    EXPECT_EQ(announcements, 2U);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, BindAndUnbindWithoutOptionalCallbacks)
{
    small_mock backend;
    EXPECT_EQ(backend.bind(1024U, 2000U), castle::status::not_configured);
    EXPECT_EQ(backend.unbind(1024U), castle::status::not_configured);
    ASSERT_EQ(backend.initialize(device_config(), nullptr, nullptr), castle::status::ok);
    EXPECT_EQ(backend.bind(1024U, 2000U), castle::status::not_found);
    EXPECT_EQ(backend.unbind(1024U), castle::status::not_found);
    void* handle = nullptr;
    address actual = 0U;
    ASSERT_EQ(backend.create_endpoint("quiet", rpmsg_addr_any, rpmsg_addr_any, nullptr,
                                      &accept_message, nullptr, handle, actual), castle::status::ok);
    EXPECT_EQ(backend.bind(actual, rpmsg_addr_any), castle::status::invalid_argument);
    EXPECT_EQ(backend.bind(999U, 2000U), castle::status::not_found);
    EXPECT_EQ(backend.unbind(actual), castle::status::not_found);
    EXPECT_EQ(backend.unbind(999U), castle::status::not_found);
    ASSERT_EQ(backend.bind(actual, 2000U), castle::status::ok);
    EXPECT_EQ(backend.remote_address(handle), 2000U);
    EXPECT_EQ(backend.unbind(actual), castle::status::ok);
    EXPECT_EQ(backend.remote_address(handle), rpmsg_addr_any);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgMockBackend, UnbindInvokesEndpointAndNameServiceCallbacks)
{
    small_mock backend;
    uint32_t announcements = 0U;
    uint32_t unbinds = 0U;
    ASSERT_EQ(backend.initialize(device_config(), &announcements, &count_name_service), castle::status::ok);
    void* handle = nullptr;
    address actual = 0U;
    ASSERT_EQ(backend.create_endpoint("svc", rpmsg_addr_any, rpmsg_addr_any, &unbinds,
                                      &accept_message, &count_unbind, handle, actual), castle::status::ok);
    ASSERT_EQ(backend.announce("svc", 2000U), castle::status::ok);
    EXPECT_EQ(announcements, 0U);
    EXPECT_EQ(backend.unbind(actual), castle::status::ok);
    EXPECT_EQ(unbinds, 1U);
    EXPECT_EQ(announcements, 1U);
    EXPECT_EQ(backend.unbind(actual), castle::status::not_found);
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

