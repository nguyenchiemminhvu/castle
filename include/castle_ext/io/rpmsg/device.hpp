// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file device.hpp
 * @brief Fixed-capacity RPMsg device facade shared by OpenAMP-based platforms.
 *
 * `basic_device<Backend, Options>` owns no transport memory: the backend owns or
 * references its OpenAMP/VirtIO resources. CASTLE owns only fixed endpoint and
 * service tables plus inline callback storage.
 */
#ifndef CASTLE_EXT_IO_RPMSG_DEVICE_HPP
#define CASTLE_EXT_IO_RPMSG_DEVICE_HPP

#include "castle/callbacks/function.hpp"
#include "castle/container/string.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/forward.hpp"
#include "castle_ext/io/rpmsg/backend.hpp"
#include "castle_ext/io/rpmsg/endpoint.hpp"
#include "castle_ext/io/rpmsg/message.hpp"
#include "castle_ext/io/rpmsg/name_service.hpp"
#include "castle_ext/io/rpmsg/types.hpp"

#include <stdint.h>

namespace castle
{
namespace io
{
namespace rpmsg
{

template <typename Backend, typename Options = options<> >
class basic_device
{
    static_assert(detail::has_backend_contract<Backend>::value,
                  "RPMsg Backend does not satisfy the CASTLE backend contract");

public:
    using backend_type = Backend;
    using options_type = Options;
    using endpoint_type = basic_endpoint<Backend, Options>;
    using receive_callback = typename endpoint_type::receive_callback;
    using unbind_callback = typename endpoint_type::unbind_callback;
    using name_service_callback = castle::callbacks::function<void(name_service_event CASTLE_CONST&), Options::callback_storage>;
    using service_name_type = container::string<Options::max_name_length>;

    basic_device() CASTLE_DELETE;

    /** @brief Binds a device facade to an externally owned backend. */
    explicit basic_device(Backend& backend) CASTLE_NOEXCEPT
        : backend_(backend)
        , initialized_(false)
        , config_(Options::default_role, nullptr, 0U, Options::buffer_count, Options::buffer_size)
        , last_name_service_status_(castle::status::ok)
    {
    }

    basic_device(basic_device CASTLE_CONST&) CASTLE_DELETE;
    basic_device& operator=(basic_device CASTLE_CONST&) CASTLE_DELETE;
    basic_device(basic_device&&) CASTLE_DELETE;
    basic_device& operator=(basic_device&&) CASTLE_DELETE;

    ~basic_device()
    {
        if (initialized_)
        {
            (void)shutdown();
        }
    }

    /** @brief Initializes the backend using the compile-time default role and pool dimensions. */
    castle::status initialize() CASTLE_NOEXCEPT
    {
        return initialize(config_);
    }

    /** @brief Initializes the backend with platform-provided shared-memory and role configuration. */
    castle::status initialize(device_config CASTLE_CONST& config) CASTLE_NOEXCEPT
    {
        if (initialized_ || backend_.is_initialized())
        {
            return castle::status::invalid_config;
        }
        castle::status result = validate_config(config);
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (config.buffer_size < Options::max_payload_size + rpmsg_header_size
            || config.buffer_count < 1U
            || config.buffer_count > Options::buffer_count
            || config.buffer_size > Options::buffer_size)
        {
            return castle::status::invalid_config;
        }
        result = backend_.initialize(config, this, &name_service_trampoline);
        if (castle::succeeded(result))
        {
            config_ = config;
            initialized_ = true;
            last_name_service_status_ = castle::status::ok;
        }
        return result;
    }

    /** @brief Destroys all active endpoints, then deinitializes the backend. */
    castle::status shutdown() CASTLE_NOEXCEPT
    {
        if (!initialized_)
        {
            return castle::status::not_configured;
        }
        for (size_type i = 0U; i < Options::max_endpoints; ++i)
        {
            endpoint_slot& slot = endpoints_[i];
            if (slot.active)
            {
                void* handle = slot.backend_handle;
                slot.active = false;
                if (handle != nullptr)
                {
                    backend_.destroy_endpoint(handle);
                }
                release_slot(slot);
            }
        }
        for (size_type i = 0U; i < Options::max_channels; ++i)
        {
            channels_[i].active = false;
            channels_[i].name.clear();
            channels_[i].endpoint_address = rpmsg_addr_any;
        }
        initialized_ = false;
        return backend_.shutdown();
    }

    /** @brief Reports whether both this facade and its backend are initialized. */
    CASTLE_NODISCARD bool is_initialized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return initialized_ && backend_.is_initialized();
    }

    /** @brief Returns the runtime configuration accepted by the last successful initialize(). */
    CASTLE_NODISCARD device_config CASTLE_CONST& configuration() CASTLE_CONST CASTLE_NOEXCEPT { return config_; }

    /** @brief Installs a callback for Name Service events, including unclaimed CREATE and DESTROY. */
    template <typename Callable>
    castle::status set_name_service_callback(Callable&& callback) CASTLE_NOEXCEPT
    {
        name_service_callback candidate(CASTLE_FORWARD<Callable>(callback));
        if (!candidate)
        {
            return castle::status::invalid_callback;
        }
        name_service_callback_ = CASTLE_MOVE(candidate);
        return castle::status::ok;
    }

    /** @brief Clears the unclaimed Name Service callback. */
    void clear_name_service_callback() CASTLE_NOEXCEPT { name_service_callback_ = nullptr; }

    /** @brief Processes backend progress/notifications. Platform adapters may implement this as a no-op. */
    castle::status poll() CASTLE_NOEXCEPT
    {
        if (!is_initialized())
        {
            return castle::status::not_configured;
        }
        return backend_.poll();
    }

    /** @brief Number of active CASTLE endpoints. Complexity is O(MaxEndpoints). */
    CASTLE_NODISCARD size_type endpoint_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type count = 0U;
        for (size_type i = 0U; i < Options::max_endpoints; ++i)
        {
            if (endpoints_[i].active)
            {
                ++count;
            }
        }
        return count;
    }

    /** @brief Number of cached remote service announcements. Complexity is O(MaxChannels). */
    CASTLE_NODISCARD size_type service_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type count = 0U;
        for (size_type i = 0U; i < Options::max_channels; ++i)
        {
            if (channels_[i].active)
            {
                ++count;
            }
        }
        return count;
    }

    /** @brief Looks up a cached Name Service record by name and returns its remote endpoint address. */
    castle::status find_service(char CASTLE_CONST* name, address& remote) CASTLE_CONST CASTLE_NOEXCEPT
    {
        remote = rpmsg_addr_any;
        if (name == nullptr || name[0] == '\0')
        {
            return castle::status::invalid_argument;
        }
        for (size_type i = 0U; i < Options::max_channels; ++i)
        {
            if (channels_[i].active && names_equal(channels_[i].name.c_str(), name))
            {
                remote = channels_[i].endpoint_address;
                return castle::status::ok;
            }
        }
        return castle::status::not_found;
    }

    /** @brief Last result of updating the bounded Name Service table. */
    CASTLE_NODISCARD castle::status last_name_service_status() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return last_name_service_status_;
    }

    /** @brief Explicitly applies a decoded NS event (useful for transports without an NS callback). */
    castle::status process_name_service_event(name_service_event CASTLE_CONST& event) CASTLE_NOEXCEPT
    {
        if (!is_initialized())
        {
            return castle::status::not_configured;
        }
        castle::status result = update_channel_table(event);
        last_name_service_status_ = result;
        if (result == castle::status::invalid_argument)
        {
            return result;
        }
        for (size_type i = 0U; i < Options::max_endpoints; ++i)
        {
            endpoint_slot& slot = endpoints_[i];
            if (slot.active && names_equal(slot.name.c_str(), event.name.c_str()))
            {
                address backend_destination = backend_.remote_address(slot.backend_handle);
                address effective_destination = backend_destination == rpmsg_addr_any
                                               ? slot.destination : backend_destination;
                if (event.action == name_service_action::create)
                {
                    if (effective_destination == rpmsg_addr_any)
                    {
                        slot.destination = event.endpoint_address;
                    }
                }
                else if (effective_destination == event.endpoint_address)
                {
                    slot.destination = rpmsg_addr_any;
                }
            }
        }
        if (Options::auto_bind && name_service_callback_)
        {
            name_service_callback_(event);
        }
        return result;
    }

    /** @brief Returns the backend for platform-specific diagnostics or integration hooks. */
    Backend& backend() CASTLE_NOEXCEPT { return backend_; }
    Backend CASTLE_CONST& backend() CASTLE_CONST CASTLE_NOEXCEPT { return backend_; }

private:
    friend class basic_endpoint<Backend, Options>;

    struct endpoint_slot
    {
        bool active;
        uint32_t generation;
        service_name_type name;
        address source;
        address destination;
        void* backend_handle;
        receive_callback on_receive;
        unbind_callback on_unbind;
        basic_device* owner;
        uint32_t dispatching;   // callbacks of this slot currently on the call stack
        bool zombie;            // destroyed from inside a callback; storage freed when dispatching reaches 0

        endpoint_slot() CASTLE_NOEXCEPT
            : active(false)
            , generation(1U)
            , name()
            , source(rpmsg_addr_any)
            , destination(rpmsg_addr_any)
            , backend_handle(nullptr)
            , on_receive()
            , on_unbind()
            , owner(nullptr)
            , dispatching(0U)
            , zombie(false)
        {
        }
    };

    struct channel_slot
    {
        bool active;
        service_name_type name;
        address endpoint_address;

        channel_slot() CASTLE_NOEXCEPT
            : active(false)
            , name()
            , endpoint_address(rpmsg_addr_any)
        {
        }
    };

    static bool names_equal(char CASTLE_CONST* lhs, char CASTLE_CONST* rhs) CASTLE_NOEXCEPT
    {
        if (lhs == nullptr || rhs == nullptr)
        {
            return false;
        }
        size_type i = 0U;
        while (lhs[i] != '\0' && rhs[i] != '\0')
        {
            if (lhs[i] != rhs[i])
            {
                return false;
            }
            ++i;
        }
        return lhs[i] == rhs[i];
    }

    static castle::status receive_trampoline(void* context, message_view CASTLE_CONST& message) CASTLE_NOEXCEPT
    {
        endpoint_slot* slot = static_cast<endpoint_slot*>(context);
        if (slot == nullptr || !slot->active || slot->owner == nullptr)
        {
            return castle::status::not_configured;
        }
        if (!slot->on_receive)
        {
            return castle::status::invalid_callback;
        }
        if (message.size > Options::max_payload_size || (message.data == nullptr && message.size != 0U))
        {
            return castle::status::data_loss;
        }
        ++slot->dispatching;
        slot->on_receive(message);
        end_dispatch(*slot);
        return castle::status::ok;
    }

    static void unbind_trampoline(void* context) CASTLE_NOEXCEPT
    {
        endpoint_slot* slot = static_cast<endpoint_slot*>(context);
        if (slot == nullptr || !slot->active)
        {
            return;
        }
        slot->destination = rpmsg_addr_any;
        if (slot->on_unbind)
        {
            ++slot->dispatching;
            slot->on_unbind();
            end_dispatch(*slot);
        }
    }

    static void name_service_trampoline(void* context, name_service_event CASTLE_CONST& event) CASTLE_NOEXCEPT
    {
        basic_device* self = static_cast<basic_device*>(context);
        if (self != nullptr)
        {
            (void)self->process_name_service_event(event);
        }
    }

    static void clear_slot(endpoint_slot& slot) CASTLE_NOEXCEPT
    {
        slot.active = false;
        slot.name.clear();
        slot.source = rpmsg_addr_any;
        slot.destination = rpmsg_addr_any;
        slot.backend_handle = nullptr;
        slot.on_receive = nullptr;
        slot.on_unbind = nullptr;
        slot.owner = nullptr;
        slot.zombie = false;
        slot.generation = slot.generation + 1U;
        if (slot.generation == 0U)
        {
            slot.generation = 1U;
        }
    }

    // A callback may destroy its own endpoint; its storage must outlive the running call.
    static void release_slot(endpoint_slot& slot) CASTLE_NOEXCEPT
    {
        slot.active = false;
        if (slot.dispatching == 0U)
        {
            clear_slot(slot);
        }
        else
        {
            slot.zombie = true;
        }
    }

    static void end_dispatch(endpoint_slot& slot) CASTLE_NOEXCEPT
    {
        --slot.dispatching;
        if (slot.dispatching == 0U && slot.zombie)
        {
            clear_slot(slot);
        }
    }

    castle::status validate_service_name(char CASTLE_CONST* name, service_name_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (name == nullptr || name[0] == '\0')
        {
            return castle::status::invalid_argument;
        }
        size_type length = 0U;
        while (name[length] != '\0')
        {
            ++length;
            if (length > Options::max_name_length)
            {
                return castle::status::out_of_range;
            }
        }
        return out.assign(name);
    }

    castle::status attach_endpoint(endpoint_type& endpoint,
                                   char CASTLE_CONST* service_name,
                                   address local_address,
                                   address remote_address,
                                   receive_callback&& on_receive,
                                   unbind_callback&& on_unbind) CASTLE_NOEXCEPT
    {
        if (!is_initialized())
        {
            return castle::status::not_configured;
        }
        if (endpoint.device_ != nullptr)
        {
            return castle::status::already_exists;
        }
        if (!is_valid_local_address(local_address)
            || (remote_address != rpmsg_addr_any && is_reserved_address(remote_address)))
        {
            return castle::status::invalid_argument;
        }
        if (!on_receive)
        {
            return castle::status::invalid_callback;
        }
        service_name_type name;
        castle::status result = validate_service_name(service_name, name);
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (local_address != rpmsg_addr_any)
        {
            for (size_type i = 0U; i < Options::max_endpoints; ++i)
            {
                if (endpoints_[i].active && endpoints_[i].source == local_address)
                {
                    return castle::status::already_exists;
                }
            }
        }

        size_type free_slot = Options::max_endpoints;
        for (size_type i = 0U; i < Options::max_endpoints; ++i)
        {
            if (!endpoints_[i].active && !endpoints_[i].zombie)
            {
                free_slot = i;
                break;
            }
        }
        if (free_slot == Options::max_endpoints)
        {
            return castle::status::full;
        }

        endpoint_slot& slot = endpoints_[free_slot];
        slot.active = true;
        slot.owner = this;
        slot.name = name;
        slot.source = local_address;
        slot.destination = remote_address;
        slot.on_receive = CASTLE_MOVE(on_receive);
        slot.on_unbind = CASTLE_MOVE(on_unbind);
        void* backend_handle = nullptr;
        address actual_source = rpmsg_addr_any;
        result = backend_.create_endpoint(slot.name.c_str(), local_address, remote_address,
                                          &slot, &receive_trampoline, &unbind_trampoline,
                                          backend_handle, actual_source);
        if (!castle::succeeded(result))
        {
            clear_slot(slot);
            return result;
        }
        if (backend_handle == nullptr || actual_source == rpmsg_addr_any || is_reserved_address(actual_source))
        {
            if (backend_handle != nullptr)
            {
                backend_.destroy_endpoint(backend_handle);
            }
            clear_slot(slot);
            return castle::status::invalid_config;
        }
        slot.backend_handle = backend_handle;
        slot.source = actual_source;
        endpoint.device_ = this;
        endpoint.slot_ = free_slot;
        endpoint.generation_ = slot.generation;
        return castle::status::ok;
    }

    castle::status detach_endpoint(endpoint_type& endpoint) CASTLE_NOEXCEPT
    {
        if (!is_endpoint_valid(endpoint))
        {
            endpoint.reset();
            return castle::status::not_found;
        }
        endpoint_slot& slot = endpoints_[endpoint.slot_];
        void* handle = slot.backend_handle;
        address remote = handle == nullptr ? slot.destination : backend_.remote_address(handle);
        if (remote == rpmsg_addr_any)
        {
            remote = slot.destination;
        }
        slot.active = false;
        if (handle != nullptr)
        {
            backend_.destroy_endpoint(handle);
        }
        if (remote != rpmsg_addr_any)
        {
            remove_channel(slot.name.c_str(), remote);
        }
        release_slot(slot);
        endpoint.reset();
        return castle::status::ok;
    }

    bool is_endpoint_valid(endpoint_type CASTLE_CONST& endpoint) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return endpoint.device_ == this
            && endpoint.slot_ < Options::max_endpoints
            && endpoints_[endpoint.slot_].active
            && endpoints_[endpoint.slot_].generation == endpoint.generation_;
    }

    address endpoint_source(endpoint_type CASTLE_CONST& endpoint) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_endpoint_valid(endpoint) ? endpoints_[endpoint.slot_].source : rpmsg_addr_any;
    }

    address endpoint_destination(endpoint_type CASTLE_CONST& endpoint) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!is_endpoint_valid(endpoint))
        {
            return rpmsg_addr_any;
        }
        endpoint_slot CASTLE_CONST& slot = endpoints_[endpoint.slot_];
        address current = backend_.remote_address(slot.backend_handle);
        return current == rpmsg_addr_any ? slot.destination : current;
    }

    castle::status send_endpoint(endpoint_type CASTLE_CONST& endpoint,
                                 address local,
                                 address remote,
                                 void CASTLE_CONST* data,
                                 size_type length,
                                 timeout_type timeout) CASTLE_NOEXCEPT
    {
        if (!is_endpoint_valid(endpoint))
        {
            return castle::status::not_configured;
        }
        if (data == nullptr && length != 0U)
        {
            return castle::status::invalid_argument;
        }
        if (length > Options::max_payload_size)
        {
            return castle::status::out_of_range;
        }
        if ((timeout < 0 && timeout != wait_forever) || local == rpmsg_addr_any
            || !is_valid_local_address(local))
        {
            return castle::status::invalid_argument;
        }
        if (remote == rpmsg_addr_any)
        {
            return castle::status::not_configured;
        }
        if (is_reserved_address(remote))
        {
            return castle::status::invalid_argument;
        }
        return backend_.send(endpoints_[endpoint.slot_].backend_handle,
                             local, remote, data, length, timeout);
    }

    castle::status acquire_tx_buffer(endpoint_type CASTLE_CONST& endpoint, tx_buffer& out, bool wait) CASTLE_NOEXCEPT
    {
        out.reset();
        if (!Options::zero_copy)
        {
            return castle::status::not_configured;
        }
        if (!is_endpoint_valid(endpoint))
        {
            return castle::status::not_configured;
        }
        void* data = nullptr;
        size_type capacity = 0U;
        castle::status result = backend_.acquire_tx_buffer(
            endpoints_[endpoint.slot_].backend_handle, wait, data, capacity);
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (data == nullptr || capacity == 0U)
        {
            if (data != nullptr)
            {
                (void)backend_.release_tx_buffer(endpoints_[endpoint.slot_].backend_handle, data);
            }
            return castle::status::invalid_config;
        }
        out.data = static_cast<uint8_t*>(data);
        out.capacity = capacity < Options::max_payload_size ? capacity : Options::max_payload_size;
        out.endpoint_token = endpoints_[endpoint.slot_].backend_handle;
        out.active = true;
        return castle::status::ok;
    }

    castle::status send_nocopy(endpoint_type CASTLE_CONST& endpoint,
                               tx_buffer& buffer,
                               size_type length,
                               address remote) CASTLE_NOEXCEPT
    {
        if (!Options::zero_copy)
        {
            return castle::status::not_configured;
        }
        if (!is_endpoint_valid(endpoint))
        {
            return castle::status::not_configured;
        }
        void* handle = endpoints_[endpoint.slot_].backend_handle;
        if (!buffer.active || buffer.data == nullptr || buffer.endpoint_token != handle)
        {
            return castle::status::invalid_argument;
        }
        if (length > buffer.capacity || length > Options::max_payload_size)
        {
            return castle::status::out_of_range;
        }
        if (remote == rpmsg_addr_any)
        {
            remote = endpoint_destination(endpoint);
        }
        if (remote == rpmsg_addr_any)
        {
            return castle::status::not_configured;
        }
        if (is_reserved_address(remote))
        {
            return castle::status::invalid_argument;
        }
        castle::status result = backend_.send_tx_buffer(
            handle, endpoint_source(endpoint), remote, buffer.data, length);
        if (castle::succeeded(result))
        {
            buffer.reset();
        }
        return result;
    }

    castle::status release_tx_buffer(endpoint_type CASTLE_CONST& endpoint, tx_buffer& buffer) CASTLE_NOEXCEPT
    {
        if (!Options::zero_copy)
        {
            return castle::status::not_configured;
        }
        if (!is_endpoint_valid(endpoint))
        {
            return castle::status::not_configured;
        }
        void* handle = endpoints_[endpoint.slot_].backend_handle;
        if (!buffer.active || buffer.data == nullptr || buffer.endpoint_token != handle)
        {
            return castle::status::invalid_argument;
        }
        castle::status result = backend_.release_tx_buffer(handle, buffer.data);
        if (castle::succeeded(result))
        {
            buffer.reset();
        }
        return result;
    }

    castle::status update_channel_table(name_service_event CASTLE_CONST& event) CASTLE_NOEXCEPT
    {
        if (event.name.empty() || event.name.size() > Options::max_name_length
            || !is_valid_remote_address(event.endpoint_address)
            || (event.action != name_service_action::create && event.action != name_service_action::destroy))
        {
            return castle::status::invalid_argument;
        }
        size_type found = Options::max_channels;
        size_type free_slot = Options::max_channels;
        for (size_type i = 0U; i < Options::max_channels; ++i)
        {
            if (channels_[i].active
                && channels_[i].endpoint_address == event.endpoint_address
                && names_equal(channels_[i].name.c_str(), event.name.c_str()))
            {
                found = i;
                break;
            }
            if (!channels_[i].active && free_slot == Options::max_channels)
            {
                free_slot = i;
            }
        }
        if (event.action == name_service_action::destroy)
        {
            if (found == Options::max_channels)
            {
                return castle::status::not_found;
            }
            channels_[found].active = false;
            channels_[found].name.clear();
            channels_[found].endpoint_address = rpmsg_addr_any;
            return castle::status::ok;
        }
        if (found == Options::max_channels)
        {
            if (free_slot == Options::max_channels)
            {
                return castle::status::full;
            }
            found = free_slot;
        }
        channels_[found].active = true;
        channels_[found].name.assign(event.name.data());
        channels_[found].endpoint_address = event.endpoint_address;
        return castle::status::ok;
    }

    void remove_channel(char CASTLE_CONST* name, address remote) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Options::max_channels; ++i)
        {
            if (channels_[i].active && names_equal(channels_[i].name.c_str(), name)
                && (remote == rpmsg_addr_any || channels_[i].endpoint_address == remote))
            {
                channels_[i].active = false;
                channels_[i].name.clear();
                channels_[i].endpoint_address = rpmsg_addr_any;
            }
        }
    }

    Backend& backend_;
    bool initialized_;
    device_config config_;
    endpoint_slot endpoints_[Options::max_endpoints];
    channel_slot channels_[Options::max_channels];
    name_service_callback name_service_callback_;
    castle::status last_name_service_status_;
};

template <typename Backend, typename Options = options<> >
using device = basic_device<Backend, Options>;

} // namespace rpmsg
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_RPMSG_DEVICE_HPP
