#include <gtest/gtest.h>
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

using namespace castle::io::rpmsg;
using test_options = options<2U, 2U, 12U, 16U, 4U, 32U, 48U, true, true, role::remote>;
using test_backend = mock::basic_backend<2U, 16U>;
using test_device = device<test_backend, test_options>;
using test_endpoint = endpoint<test_backend, test_options>;

TEST(RpmsgEndpoint, LifecycleSendReceiveAndZeroCopy)
{
    test_backend backend;
    test_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    test_endpoint ept;
    uint32_t received = 0U;
    uint32_t unbound = 0U;
    ASSERT_EQ(ept.create(dev, "demo.service", rpmsg_addr_any, rpmsg_addr_any,
        [&received](message_view const& message) { received += static_cast<uint32_t>(message.size); },
        [&unbound]() { ++unbound; }), castle::status::ok);
    EXPECT_TRUE(ept.is_created());
    EXPECT_GE(ept.source_address(), reserved_address_count);
    EXPECT_EQ(ept.destination_address(), rpmsg_addr_any);
    EXPECT_EQ(ept.send("x", 1U), castle::status::not_configured);
    ASSERT_EQ(backend.bind(ept.source_address(), 7000U), castle::status::ok);
    const uint8_t bytes[] = {1U, 2U, 3U};
    EXPECT_EQ(ept.try_send(bytes, sizeof(bytes)), castle::status::ok);
    EXPECT_EQ(backend.last_packet().destination, 7000U);
    EXPECT_EQ(ept.sendto(bytes, sizeof(bytes), 7001U, no_wait), castle::status::ok);
    EXPECT_EQ(backend.last_packet().destination, 7001U);
    EXPECT_EQ(ept.try_sendto(bytes, sizeof(bytes), 7002U), castle::status::ok);
    EXPECT_EQ(backend.last_packet().destination, 7002U);
    EXPECT_EQ(backend.send_count(), 3U);
    EXPECT_EQ(ept.try_send_to(bytes, sizeof(bytes), 35U), castle::status::invalid_argument);
    EXPECT_EQ(ept.try_send(nullptr, 1U), castle::status::invalid_argument);
    EXPECT_EQ(ept.try_send(bytes, 17U), castle::status::out_of_range);
    EXPECT_EQ(ept.send(bytes, 1U, -2), castle::status::invalid_argument);
    EXPECT_EQ(ept.send_offchannel(bytes, 1U, 1U, 7000U), castle::status::invalid_argument);
    EXPECT_EQ(backend.inject(ept.source_address(), 8000U, bytes, sizeof(bytes)), castle::status::ok);
    EXPECT_EQ(received, 3U);
    tx_buffer tx;
    ASSERT_EQ(ept.acquire_tx_buffer(tx), castle::status::ok);
    ASSERT_TRUE(tx.active);
    tx.data[0] = 0xABU;
    EXPECT_EQ(ept.send_nocopy(tx, tx.capacity + 1U), castle::status::out_of_range);
    EXPECT_TRUE(tx.active);
    EXPECT_EQ(ept.release_tx_buffer(tx), castle::status::ok);
    EXPECT_FALSE(tx.active);
    ASSERT_EQ(ept.acquire_tx_buffer(tx), castle::status::ok);
    tx.data[0] = 0xCDU;
    EXPECT_EQ(ept.send_nocopy(tx, 1U, 7000U), castle::status::ok);
    EXPECT_FALSE(tx.active);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_FALSE(ept.is_created());
    EXPECT_EQ(ept.destroy(), castle::status::not_configured);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
    (void)unbound;
}

TEST(RpmsgEndpoint, ValidationAndCapacity)
{
    test_backend backend;
    test_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    test_endpoint a;
    test_endpoint b;
    EXPECT_EQ(a.create(dev, "demo", 5U, rpmsg_addr_any, [](message_view const&) {}), castle::status::invalid_argument);
    EXPECT_EQ(a.create(dev, "demo", rpmsg_addr_any, 35U, [](message_view const&) {}), castle::status::invalid_argument);
    EXPECT_EQ(a.create(dev, "name-is-way-too-long", rpmsg_addr_any, rpmsg_addr_any, [](message_view const&) {}), castle::status::out_of_range);
    EXPECT_EQ(a.create(dev, "demo", rpmsg_addr_any, rpmsg_addr_any, nullptr), castle::status::invalid_callback);
    ASSERT_EQ(a.create(dev, "demo", 1024U, rpmsg_addr_any, [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(b.create(dev, "demo2", 1024U, rpmsg_addr_any, [](message_view const&) {}), castle::status::already_exists);
    ASSERT_EQ(b.create(dev, "demo2", rpmsg_addr_any, rpmsg_addr_any, [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(dev.endpoint_count(), 2U);
    test_endpoint full;
    EXPECT_EQ(full.create(dev, "third", rpmsg_addr_any, rpmsg_addr_any, [](message_view const&) {}), castle::status::full);
    EXPECT_EQ(a.destroy(), castle::status::ok);
    EXPECT_EQ(b.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgEndpoint, MoveAndUnsupportedZeroCopy)
{
    test_backend backend;
    test_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    test_endpoint first;
    ASSERT_EQ(first.create(dev, "move", rpmsg_addr_any, rpmsg_addr_any, [](message_view const&) {}), castle::status::ok);
    test_endpoint moved(castle::move(first));
    EXPECT_FALSE(first.is_created());
    EXPECT_TRUE(moved.is_created());
    EXPECT_EQ(moved.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);

    using no_copy_options = options<1U, 1U, 12U, 16U, 4U, 32U, 48U, true, false>;
    using no_copy_device = device<test_backend, no_copy_options>;
    using no_copy_endpoint = endpoint<test_backend, no_copy_options>;
    test_backend backend2;
    no_copy_device dev2(backend2);
    ASSERT_EQ(dev2.initialize(), castle::status::ok);
    no_copy_endpoint endpoint2;
    ASSERT_EQ(endpoint2.create(dev2, "no.copy", rpmsg_addr_any, 8000U, [](message_view const&) {}), castle::status::ok);
    tx_buffer buffer;
    EXPECT_EQ(endpoint2.acquire_tx_buffer(buffer), castle::status::not_configured);
    EXPECT_EQ(endpoint2.release_tx_buffer(buffer), castle::status::not_configured);
    EXPECT_EQ(endpoint2.destroy(), castle::status::ok);
    EXPECT_EQ(dev2.shutdown(), castle::status::ok);
}


TEST(RpmsgEndpoint, DestructorUnregistersEndpoint)
{
    test_backend backend;
    test_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    {
        test_endpoint scoped;
        ASSERT_EQ(scoped.create(dev, "scoped", rpmsg_addr_any, rpmsg_addr_any,
                                [](message_view const&) {}), castle::status::ok);
        EXPECT_EQ(dev.endpoint_count(), 1U);
    }
    EXPECT_EQ(dev.endpoint_count(), 0U);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgEndpoint, UncreatedHandleReportsNotConfigured)
{
    test_endpoint idle;
    tx_buffer tx;
    const uint8_t bytes[] = {1U};
    EXPECT_FALSE(idle.is_created());
    EXPECT_EQ(idle.source_address(), rpmsg_addr_any);
    EXPECT_EQ(idle.destination_address(), rpmsg_addr_any);
    EXPECT_EQ(idle.send(bytes, sizeof(bytes)), castle::status::not_configured);
    EXPECT_EQ(idle.send_to(bytes, sizeof(bytes), 7000U), castle::status::not_configured);
    EXPECT_EQ(idle.send_offchannel(bytes, sizeof(bytes), 1024U, 7000U), castle::status::not_configured);
    EXPECT_EQ(idle.acquire_tx_buffer(tx), castle::status::not_configured);
    EXPECT_FALSE(tx.active);
    EXPECT_EQ(idle.send_nocopy(tx, 1U), castle::status::not_configured);
    EXPECT_EQ(idle.release_tx_buffer(tx), castle::status::not_configured);
}

TEST(RpmsgEndpoint, CreateTwiceReportsAlreadyExists)
{
    test_backend backend;
    test_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    test_endpoint ept;
    ASSERT_EQ(ept.create(dev, "twice", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(ept.create(dev, "twice", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::already_exists);
    EXPECT_EQ(dev.endpoint_count(), 1U);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgEndpoint, DestroyFromOwnReceiveCallbackKeepsSlotReserved)
{
    test_backend backend;
    test_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    test_endpoint first;
    test_endpoint second;
    test_endpoint third;
    castle::status third_during = castle::status::ok;
    uint32_t size_after_destroy = 0U;
    ASSERT_EQ(first.create(dev, "self", rpmsg_addr_any, rpmsg_addr_any,
        [&](message_view const& message) {
            EXPECT_EQ(first.destroy(), castle::status::ok);
            EXPECT_FALSE(first.is_created());
            EXPECT_EQ(second.create(dev, "other", rpmsg_addr_any, rpmsg_addr_any,
                                    [](message_view const&) {}), castle::status::ok);
            // The slot of the running callback must not be handed out yet.
            third_during = third.create(dev, "third", rpmsg_addr_any, rpmsg_addr_any,
                                        [](message_view const&) {});
            size_after_destroy = static_cast<uint32_t>(message.size);
        }), castle::status::ok);
    address local = first.source_address();
    const uint8_t bytes[] = {1U, 2U, 3U};
    EXPECT_EQ(backend.inject(local, 7000U, bytes, sizeof(bytes)), castle::status::ok);
    EXPECT_EQ(third_during, castle::status::full);
    EXPECT_EQ(size_after_destroy, 3U);
    EXPECT_EQ(dev.endpoint_count(), 1U);
    EXPECT_EQ(third.create(dev, "third", rpmsg_addr_any, rpmsg_addr_any,
                           [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(dev.endpoint_count(), 2U);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgEndpoint, DestroyFromOwnUnbindCallbackIsSafe)
{
    test_backend backend;
    test_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    test_endpoint ept;
    uint32_t unbound = 0U;
    ASSERT_EQ(ept.create(dev, "unbind.self", rpmsg_addr_any, rpmsg_addr_any,
        [](message_view const&) {},
        [&]() { ++unbound; EXPECT_EQ(ept.destroy(), castle::status::ok); }), castle::status::ok);
    address local = ept.source_address();
    ASSERT_EQ(backend.bind(local, 7000U), castle::status::ok);
    EXPECT_EQ(backend.unbind(local), castle::status::ok);
    EXPECT_EQ(unbound, 1U);
    EXPECT_EQ(dev.endpoint_count(), 0U);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}
