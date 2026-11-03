#include <gtest/gtest.h>
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

using namespace castle::io::rpmsg;
using small_options = options<2U, 2U, 12U, 16U, 4U, 32U, 48U, true, true, role::remote>;
using small_backend = mock::basic_backend<2U, 16U>;
using small_device = device<small_backend, small_options>;
using small_endpoint = endpoint<small_backend, small_options>;

namespace
{
// Mock backend with injectable failures and captured dispatch pointers for stale-callback tests.
struct scripted_backend : small_backend
{
    bool fail_init = false;
    castle::status create_status = castle::status::ok;
    bool null_handle = false;
    bool force_source = false;
    address forced_source = rpmsg_addr_any;
    int acquire_mode = 0;
    castle::size_type capacity_limit = 0U;
    receive_dispatch saved_receive = nullptr;
    unbind_dispatch saved_unbind = nullptr;
    name_service_dispatch saved_name_service = nullptr;
    void* saved_context = nullptr;
    void* saved_name_service_context = nullptr;

    castle::status initialize(device_config const& config, void* context,
                              name_service_dispatch callback) CASTLE_NOEXCEPT
    {
        if (fail_init)
        {
            return castle::status::system_call_error;
        }
        saved_name_service = callback;
        saved_name_service_context = context;
        return small_backend::initialize(config, context, callback);
    }

    castle::status create_endpoint(char const* name, address src, address dst, void* context,
                                   receive_dispatch receive, unbind_dispatch unbind,
                                   void*& handle, address& actual_source) CASTLE_NOEXCEPT
    {
        if (create_status != castle::status::ok)
        {
            handle = nullptr;
            actual_source = rpmsg_addr_any;
            return create_status;
        }
        castle::status result = small_backend::create_endpoint(name, src, dst, context,
                                                               receive, unbind, handle, actual_source);
        if (castle::succeeded(result))
        {
            saved_receive = receive;
            saved_unbind = unbind;
            saved_context = context;
            if (null_handle)
            {
                handle = nullptr;
            }
            if (force_source)
            {
                actual_source = forced_source;
            }
        }
        return result;
    }

    castle::status acquire_tx_buffer(void* handle, bool wait, void*& data,
                                     castle::size_type& capacity) CASTLE_NOEXCEPT
    {
        if (acquire_mode == 1)
        {
            data = nullptr;
            capacity = 0U;
            return castle::status::ok;
        }
        castle::status result = small_backend::acquire_tx_buffer(handle, wait, data, capacity);
        if (castle::succeeded(result))
        {
            if (acquire_mode == 2)
            {
                capacity = 0U;
            }
            if (capacity_limit != 0U && capacity > capacity_limit)
            {
                capacity = capacity_limit;
            }
        }
        return result;
    }
};

name_service_event make_event(char const* name, address remote, name_service_action action)
{
    name_service_event event;
    (void)event.name.assign(name);
    event.endpoint_address = remote;
    event.action = action;
    return event;
}
} // namespace

using scripted_device = device<scripted_backend, small_options>;
using scripted_endpoint = endpoint<scripted_backend, small_options>;
using narrow_options = options<2U, 2U, 12U, 8U, 4U, 32U, 48U, true, true, role::remote>;
using narrow_device = device<scripted_backend, narrow_options>;
using narrow_endpoint = endpoint<scripted_backend, narrow_options>;

TEST(RpmsgDevice, InitializePollAndShutdownErrors)
{
    small_backend backend;
    small_device dev(backend);
    EXPECT_EQ(dev.poll(), castle::status::not_configured);
    EXPECT_EQ(dev.shutdown(), castle::status::not_configured);
    EXPECT_EQ(dev.initialize(device_config(static_cast<role>(8U))), castle::status::invalid_config);
    EXPECT_EQ(dev.initialize(device_config(role::remote, nullptr, 0U, 4U, 31U)), castle::status::invalid_config);
    EXPECT_EQ(dev.initialize(device_config(role::remote, nullptr, 0U, 5U, 32U)), castle::status::invalid_config);
    EXPECT_EQ(dev.initialize(device_config(role::remote, nullptr, 0U, 4U, 33U)), castle::status::invalid_config);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    EXPECT_TRUE(dev.is_initialized());
    EXPECT_EQ(dev.initialize(), castle::status::invalid_config);
    EXPECT_EQ(dev.poll(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
    EXPECT_FALSE(dev.is_initialized());
}

TEST(RpmsgDevice, NameServiceCacheAutoBindAndCallbacks)
{
    small_backend backend;
    small_device dev(backend);
    uint32_t callback_count = 0U;
    ASSERT_EQ(dev.set_name_service_callback([&callback_count](name_service_event const&) { ++callback_count; }), castle::status::ok);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint ept;
    uint32_t unbind_count = 0U;
    ASSERT_EQ(ept.create(dev, "demo.service", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}, [&unbind_count]() { ++unbind_count; }), castle::status::ok);
    name_service_event event;
    ASSERT_EQ(event.name.assign("demo.service"), castle::status::ok);
    event.endpoint_address = 6000U;
    event.action = name_service_action::create;
    EXPECT_EQ(backend.announce("demo.service", 6000U), castle::status::ok);
    EXPECT_EQ(callback_count, 0U); // OpenAMP binds an already-registered endpoint directly.
    EXPECT_EQ(ept.destination_address(), 6000U);
    EXPECT_EQ(dev.service_count(), 0U);
    address remote = rpmsg_addr_any;
    EXPECT_EQ(dev.find_service("demo.service", remote), castle::status::not_found);
    EXPECT_EQ(remote, rpmsg_addr_any);
    EXPECT_EQ(backend.unbind(ept.source_address()), castle::status::ok);
    EXPECT_EQ(ept.destination_address(), rpmsg_addr_any);
    EXPECT_EQ(unbind_count, 1U);
    EXPECT_EQ(callback_count, 1U); // Device-level DESTROY callback is delivered too.
    EXPECT_EQ(dev.service_count(), 0U);
    EXPECT_EQ(dev.find_service(nullptr, remote), castle::status::invalid_argument);

    EXPECT_EQ(backend.announce("orphan.svc", 6100U), castle::status::ok);
    EXPECT_EQ(callback_count, 2U);
    EXPECT_EQ(dev.service_count(), 1U);
    EXPECT_EQ(dev.find_service("orphan.svc", remote), castle::status::ok);
    EXPECT_EQ(remote, 6100U);
    EXPECT_EQ(dev.find_service("missing", remote), castle::status::not_found);
    EXPECT_EQ(remote, rpmsg_addr_any);
    event.action = name_service_action::destroy;
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::not_found);
    EXPECT_EQ(dev.last_name_service_status(), castle::status::not_found);
    EXPECT_EQ(callback_count, 3U);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, ServiceTableBoundsAndValidation)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    name_service_event event;
    ASSERT_EQ(event.name.assign("one"), castle::status::ok);
    event.endpoint_address = 2000U;
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::ok);
    ASSERT_EQ(event.name.assign("two"), castle::status::ok);
    event.endpoint_address = 2001U;
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::ok);
    ASSERT_EQ(event.name.assign("three"), castle::status::ok);
    event.endpoint_address = 2002U;
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::full);
    EXPECT_EQ(dev.service_count(), 2U);
    event.endpoint_address = rpmsg_addr_any;
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::invalid_argument);
    event.endpoint_address = 1000U;
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::invalid_argument);
    event.endpoint_address = 2002U;
    event.action = static_cast<name_service_action>(8U);
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::invalid_argument);
    event.action = name_service_action::create;
    ASSERT_EQ(event.name.assign("this-name-is-too-long"), castle::status::ok);
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::invalid_argument);
    dev.clear_name_service_callback();
    EXPECT_EQ(dev.set_name_service_callback(nullptr), castle::status::invalid_callback);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, ExplicitConfigAndNoAutoBind)
{
    using manual_options = options<1U, 1U, 12U, 16U, 4U, 32U, 48U, false, false>;
    using manual_device = device<small_backend, manual_options>;
    small_backend backend;
    manual_device dev(backend);
    uint32_t callback_count = 0U;
    ASSERT_EQ(dev.set_name_service_callback([&callback_count](name_service_event const&) { ++callback_count; }), castle::status::ok);
    ASSERT_EQ(dev.initialize(device_config(role::host, nullptr, 0U, 4U, 32U)), castle::status::ok);
    name_service_event event;
    ASSERT_EQ(event.name.assign("manual"), castle::status::ok);
    event.endpoint_address = 9000U;
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::ok);
    EXPECT_EQ(callback_count, 0U);
    EXPECT_EQ(dev.configuration().node_role, role::host);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}


TEST(RpmsgDevice, DynamicEndpointCreationFromNameService)
{
    small_backend backend;
    small_device dev(backend);
    small_endpoint dynamic_endpoint;
    ASSERT_EQ(dev.set_name_service_callback(
        [&dev, &dynamic_endpoint](name_service_event const& event)
        {
            (void)dynamic_endpoint.create(dev, event.name.c_str(), rpmsg_addr_any,
                                          event.endpoint_address, [](message_view const&) {});
        }), castle::status::ok);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    EXPECT_EQ(backend.announce("dynamic", 7100U), castle::status::ok);
    EXPECT_TRUE(dynamic_endpoint.is_created());
    EXPECT_EQ(dynamic_endpoint.destination_address(), 7100U);
    EXPECT_EQ(dev.endpoint_count(), 1U);
    EXPECT_EQ(dev.service_count(), 1U);
    EXPECT_EQ(dynamic_endpoint.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, MultipleEndpointsShareServiceName)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint first;
    small_endpoint second;
    ASSERT_EQ(first.create(dev, "shared", rpmsg_addr_any, rpmsg_addr_any,
                           [](message_view const&) {}), castle::status::ok);
    ASSERT_EQ(second.create(dev, "shared", rpmsg_addr_any, rpmsg_addr_any,
                            [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(backend.announce("shared", 7200U), castle::status::ok);
    EXPECT_EQ(first.destination_address(), 7200U);
    EXPECT_EQ(second.destination_address(), rpmsg_addr_any);
    EXPECT_EQ(backend.announce("shared", 7201U), castle::status::ok);
    EXPECT_EQ(first.destination_address(), 7200U);
    EXPECT_EQ(second.destination_address(), 7201U);
    EXPECT_EQ(first.destroy(), castle::status::ok);
    EXPECT_EQ(second.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, DestructorShutsDownInitializedDevice)
{
    small_backend backend;
    {
        small_device dev(backend);
        ASSERT_EQ(dev.initialize(), castle::status::ok);
    }
    EXPECT_FALSE(backend.is_initialized());
}

TEST(RpmsgDevice, InitializeRejectsBackendAlreadyInitialized)
{
    small_backend backend;
    ASSERT_EQ(backend.initialize(device_config(), nullptr, nullptr), castle::status::ok);
    small_device dev(backend);
    EXPECT_EQ(dev.initialize(), castle::status::invalid_config);
    EXPECT_FALSE(dev.is_initialized());
    EXPECT_EQ(backend.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, IsInitializedFollowsBackendState)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    ASSERT_EQ(backend.shutdown(), castle::status::ok);
    EXPECT_FALSE(dev.is_initialized());
    EXPECT_EQ(dev.poll(), castle::status::not_configured);
    EXPECT_EQ(dev.shutdown(), castle::status::not_configured);
}

TEST(RpmsgDevice, InitializeFailureKeepsDeviceUninitialized)
{
    scripted_backend backend;
    backend.fail_init = true;
    scripted_device dev(backend);
    EXPECT_EQ(dev.initialize(), castle::status::system_call_error);
    EXPECT_FALSE(dev.is_initialized());
    EXPECT_EQ(dev.shutdown(), castle::status::not_configured);
}

TEST(RpmsgDevice, ShutdownDetachesLiveEndpoints)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint ept;
    ASSERT_EQ(ept.create(dev, "live", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
    EXPECT_EQ(dev.endpoint_count(), 0U);
    EXPECT_FALSE(ept.is_created());
    EXPECT_EQ(ept.source_address(), rpmsg_addr_any);
    EXPECT_EQ(ept.destination_address(), rpmsg_addr_any);
    EXPECT_EQ(ept.send("x", 1U), castle::status::not_configured);
    EXPECT_EQ(ept.destroy(), castle::status::not_found);
}

TEST(RpmsgDevice, ReusedSlotInvalidatesStaleHandle)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint stale;
    ASSERT_EQ(stale.create(dev, "old", rpmsg_addr_any, rpmsg_addr_any,
                           [](message_view const&) {}), castle::status::ok);
    ASSERT_EQ(dev.shutdown(), castle::status::ok);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint fresh;
    ASSERT_EQ(fresh.create(dev, "new", rpmsg_addr_any, rpmsg_addr_any,
                           [](message_view const&) {}), castle::status::ok);
    EXPECT_FALSE(stale.is_created());
    EXPECT_EQ(stale.send("x", 1U), castle::status::not_configured);
    EXPECT_EQ(stale.destroy(), castle::status::not_found);
    EXPECT_TRUE(fresh.is_created());
    EXPECT_EQ(dev.endpoint_count(), 1U);
    EXPECT_EQ(fresh.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, AttachRejectsUninitializedDeviceAndBadNames)
{
    small_backend backend;
    small_device dev(backend);
    small_endpoint ept;
    EXPECT_EQ(ept.create(dev, "demo", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::not_configured);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    EXPECT_EQ(ept.create(dev, nullptr, rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::invalid_argument);
    EXPECT_EQ(ept.create(dev, "", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::invalid_argument);
    EXPECT_FALSE(ept.is_created());
    EXPECT_EQ(dev.endpoint_count(), 0U);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, BackendCreateFailureRollsBackSlot)
{
    scripted_backend backend;
    scripted_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    scripted_endpoint ept;
    backend.create_status = castle::status::system_call_error;
    EXPECT_EQ(ept.create(dev, "rollback", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::system_call_error);
    EXPECT_FALSE(ept.is_created());
    EXPECT_EQ(dev.endpoint_count(), 0U);
    backend.create_status = castle::status::ok;
    ASSERT_EQ(ept.create(dev, "rollback", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(dev.endpoint_count(), 1U);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, BackendHandleAndSourceValidationRejectsAttach)
{
    scripted_backend backend;
    scripted_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    scripted_endpoint ept;
    backend.null_handle = true;
    EXPECT_EQ(ept.create(dev, "nullh", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::invalid_config);
    backend.null_handle = false;
    backend.force_source = true;
    backend.forced_source = rpmsg_addr_any;
    EXPECT_EQ(ept.create(dev, "anysrc", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::invalid_config);
    backend.forced_source = 35U;
    EXPECT_EQ(ept.create(dev, "resv", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::invalid_config);
    EXPECT_EQ(dev.endpoint_count(), 0U);
    backend.force_source = false;
    ASSERT_EQ(ept.create(dev, "ok", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(dev.endpoint_count(), 1U);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, DistinctExplicitLocalAddressesCoexist)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint first;
    small_endpoint second;
    ASSERT_EQ(first.create(dev, "first", 1024U, rpmsg_addr_any,
                           [](message_view const&) {}), castle::status::ok);
    ASSERT_EQ(second.create(dev, "second", 1025U, rpmsg_addr_any,
                            [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(first.source_address(), 1024U);
    EXPECT_EQ(second.source_address(), 1025U);
    EXPECT_EQ(first.destroy(), castle::status::ok);
    EXPECT_EQ(second.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, NameServiceEventsRequireInitializedDevice)
{
    small_backend backend;
    small_device dev(backend);
    name_service_event event = make_event("svc", 6000U, name_service_action::create);
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::not_configured);
    EXPECT_EQ(dev.service_count(), 0U);
}

TEST(RpmsgDevice, NameServiceRejectsEmptyName)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    name_service_event event = make_event("", 6000U, name_service_action::create);
    EXPECT_EQ(dev.process_name_service_event(event), castle::status::invalid_argument);
    EXPECT_EQ(dev.last_name_service_status(), castle::status::invalid_argument);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, ServiceLookupMatchesWholeNamesOnly)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    name_service_event event = make_event("orphan.svc", 6100U, name_service_action::create);
    ASSERT_EQ(dev.process_name_service_event(event), castle::status::ok);
    address remote = rpmsg_addr_any;
    EXPECT_EQ(dev.find_service("", remote), castle::status::invalid_argument);
    EXPECT_EQ(dev.find_service("orphan", remote), castle::status::not_found);
    EXPECT_EQ(dev.find_service("orphan.svc.x", remote), castle::status::not_found);
    EXPECT_EQ(dev.find_service("orphan.svc", remote), castle::status::ok);
    EXPECT_EQ(remote, 6100U);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, NameServiceEventBindsUnboundEndpointOnly)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint ept;
    ASSERT_EQ(ept.create(dev, "svc", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    ASSERT_EQ(dev.process_name_service_event(make_event("svc", 6000U, name_service_action::create)),
              castle::status::ok);
    EXPECT_EQ(ept.destination_address(), 6000U);
    ASSERT_EQ(backend.bind(ept.source_address(), 7000U), castle::status::ok);
    EXPECT_EQ(dev.process_name_service_event(make_event("svc", 6100U, name_service_action::create)),
              castle::status::ok);
    EXPECT_EQ(ept.destination_address(), 7000U);
    EXPECT_EQ(dev.service_count(), 2U);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, NameServiceDestroyClearsMatchingDestination)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint ept;
    ASSERT_EQ(ept.create(dev, "svc", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    ASSERT_EQ(dev.process_name_service_event(make_event("svc", 6000U, name_service_action::create)),
              castle::status::ok);
    ASSERT_EQ(ept.destination_address(), 6000U);
    EXPECT_EQ(dev.process_name_service_event(make_event("svc", 6001U, name_service_action::destroy)),
              castle::status::not_found);
    EXPECT_EQ(ept.destination_address(), 6000U);
    EXPECT_EQ(dev.process_name_service_event(make_event("svc", 6000U, name_service_action::destroy)),
              castle::status::ok);
    EXPECT_EQ(ept.destination_address(), rpmsg_addr_any);
    EXPECT_EQ(dev.service_count(), 0U);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, ChannelTableMatchesNameAndAddress)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    ASSERT_EQ(dev.process_name_service_event(make_event("a", 2000U, name_service_action::create)),
              castle::status::ok);
    EXPECT_EQ(dev.process_name_service_event(make_event("a", 2000U, name_service_action::create)),
              castle::status::ok);
    EXPECT_EQ(dev.service_count(), 1U);
    EXPECT_EQ(dev.process_name_service_event(make_event("b", 2000U, name_service_action::create)),
              castle::status::ok);
    EXPECT_EQ(dev.service_count(), 2U);
    EXPECT_EQ(dev.process_name_service_event(make_event("a", 2001U, name_service_action::destroy)),
              castle::status::not_found);
    EXPECT_EQ(dev.process_name_service_event(make_event("b", 2000U, name_service_action::destroy)),
              castle::status::ok);
    EXPECT_EQ(dev.service_count(), 1U);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, DetachRemovesOnlyChannelForCurrentRemote)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint ept;
    ASSERT_EQ(ept.create(dev, "svc", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    ASSERT_EQ(dev.process_name_service_event(make_event("svc", 6000U, name_service_action::create)),
              castle::status::ok);
    ASSERT_EQ(dev.process_name_service_event(make_event("svc", 6001U, name_service_action::create)),
              castle::status::ok);
    EXPECT_EQ(dev.service_count(), 2U);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.service_count(), 1U);
    address remote = rpmsg_addr_any;
    EXPECT_EQ(dev.find_service("svc", remote), castle::status::ok);
    EXPECT_EQ(remote, 6001U);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, NameServiceTrampolineIgnoresMissingContext)
{
    scripted_backend backend;
    scripted_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    name_service_event event = make_event("ghost", 6000U, name_service_action::create);
    backend.saved_name_service(nullptr, event);
    EXPECT_EQ(dev.service_count(), 0U);
    backend.saved_name_service(backend.saved_name_service_context, event);
    EXPECT_EQ(dev.service_count(), 1U);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, StaleBackendCallbacksAfterDestroyAreIgnored)
{
    scripted_backend backend;
    scripted_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    uint32_t received = 0U;
    uint32_t unbound = 0U;
    scripted_endpoint ept;
    ASSERT_EQ(ept.create(dev, "gone", rpmsg_addr_any, rpmsg_addr_any,
                         [&received](message_view const& message) { received += static_cast<uint32_t>(message.size); },
                         [&unbound]() { ++unbound; }), castle::status::ok);
    receive_dispatch receive = backend.saved_receive;
    unbind_dispatch unbind = backend.saved_unbind;
    void* context = backend.saved_context;
    ASSERT_EQ(ept.destroy(), castle::status::ok);
    const uint8_t bytes[2] = {1U, 2U};
    EXPECT_EQ(receive(context, message_view(1024U, 2000U, bytes, sizeof(bytes))), castle::status::not_configured);
    unbind(context);
    EXPECT_EQ(received, 0U);
    EXPECT_EQ(unbound, 0U);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, ReceivePathValidatesPayloadSize)
{
    scripted_backend backend;
    narrow_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    narrow_endpoint ept;
    uint32_t received = 0U;
    ASSERT_EQ(ept.create(dev, "rx", rpmsg_addr_any, rpmsg_addr_any,
                         [&received](message_view const& message) { received += static_cast<uint32_t>(message.size); }),
              castle::status::ok);
    const uint8_t bytes[12] = {};
    EXPECT_EQ(backend.inject(ept.source_address(), 2000U, bytes, 12U), castle::status::data_loss);
    EXPECT_EQ(backend.inject(ept.source_address(), 2000U, bytes, 8U), castle::status::ok);
    EXPECT_EQ(received, 8U);
    EXPECT_EQ(backend.saved_receive(backend.saved_context,
                                    message_view(ept.source_address(), 2000U, nullptr, 4U)),
              castle::status::data_loss);
    EXPECT_EQ(received, 8U);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, UnbindWithoutCallbackClearsDestination)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint ept;
    ASSERT_EQ(ept.create(dev, "quiet", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    ASSERT_EQ(backend.announce("quiet", 2000U), castle::status::ok);
    EXPECT_EQ(ept.destination_address(), 2000U);
    EXPECT_EQ(backend.unbind(ept.source_address()), castle::status::ok);
    EXPECT_EQ(ept.destination_address(), rpmsg_addr_any);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, AcquireTxBufferHandlesEmptyBackendBuffers)
{
    scripted_backend backend;
    scripted_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    scripted_endpoint ept;
    ASSERT_EQ(ept.create(dev, "tx", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    tx_buffer tx;
    backend.acquire_mode = 1;
    EXPECT_EQ(ept.acquire_tx_buffer(tx), castle::status::invalid_config);
    EXPECT_FALSE(tx.active);
    backend.acquire_mode = 2;
    EXPECT_EQ(ept.acquire_tx_buffer(tx), castle::status::invalid_config);
    EXPECT_FALSE(tx.active);
    backend.acquire_mode = 0;
    ASSERT_EQ(ept.acquire_tx_buffer(tx), castle::status::ok);
    EXPECT_TRUE(tx.active);
    EXPECT_EQ(tx.capacity, 16U);
    EXPECT_EQ(ept.release_tx_buffer(tx), castle::status::ok);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, AcquireTxBufferCapacityIsClampedToDevicePayload)
{
    scripted_backend backend;
    backend.capacity_limit = 8U;
    scripted_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    scripted_endpoint ept;
    ASSERT_EQ(ept.create(dev, "clamp", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    tx_buffer tx;
    ASSERT_EQ(ept.acquire_tx_buffer(tx), castle::status::ok);
    EXPECT_EQ(tx.capacity, 8U);
    EXPECT_EQ(ept.send_nocopy(tx, 9U, 2000U), castle::status::out_of_range);
    EXPECT_TRUE(tx.active);
    EXPECT_EQ(ept.send_nocopy(tx, 8U, 2000U), castle::status::ok);
    EXPECT_FALSE(tx.active);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, SendNocopyResolvesDestinationAndReportsFailures)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint ept;
    ASSERT_EQ(ept.create(dev, "nocopy", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    tx_buffer tx;
    ASSERT_EQ(ept.acquire_tx_buffer(tx), castle::status::ok);
    EXPECT_EQ(ept.send_nocopy(tx, 1U), castle::status::not_configured);
    EXPECT_TRUE(tx.active);
    EXPECT_EQ(ept.send_nocopy(tx, 1U, 35U), castle::status::invalid_argument);
    ASSERT_EQ(backend.bind(ept.source_address(), 7000U), castle::status::ok);
    backend.fail_next_send(castle::status::system_call_error);
    EXPECT_EQ(ept.send_nocopy(tx, 1U), castle::status::system_call_error);
    EXPECT_TRUE(tx.active);
    EXPECT_EQ(ept.send_nocopy(tx, 1U), castle::status::ok);
    EXPECT_EQ(backend.last_packet().destination, 7000U);
    EXPECT_FALSE(tx.active);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

TEST(RpmsgDevice, TxBufferHandlesRejectMalformedHandles)
{
    small_backend backend;
    small_device dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    small_endpoint ept;
    ASSERT_EQ(ept.create(dev, "forged", rpmsg_addr_any, rpmsg_addr_any,
                         [](message_view const&) {}), castle::status::ok);
    ASSERT_EQ(backend.bind(ept.source_address(), 7000U), castle::status::ok);
    tx_buffer tx;
    ASSERT_EQ(ept.acquire_tx_buffer(tx), castle::status::ok);
    tx_buffer forged;
    forged.active = true;
    EXPECT_EQ(ept.send_nocopy(forged, 1U), castle::status::invalid_argument);
    EXPECT_EQ(ept.release_tx_buffer(forged), castle::status::invalid_argument);
    forged.data = tx.data;
    EXPECT_EQ(ept.send_nocopy(forged, 1U), castle::status::invalid_argument);
    EXPECT_EQ(ept.release_tx_buffer(forged), castle::status::invalid_argument);
    uint8_t foreign[16] = {};
    uint8_t* owned = tx.data;
    tx.data = foreign;
    EXPECT_EQ(ept.release_tx_buffer(tx), castle::status::invalid_argument);
    EXPECT_TRUE(tx.active);
    tx.data = owned;
    EXPECT_EQ(ept.release_tx_buffer(tx), castle::status::ok);
    EXPECT_FALSE(tx.active);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}
