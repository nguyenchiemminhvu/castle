// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mock.hpp
 * @brief Deterministic in-memory backend for RPMsg tests and desktop samples.
 */
#ifndef CASTLE_EXT_IO_RPMSG_BACKENDS_MOCK_HPP
#define CASTLE_EXT_IO_RPMSG_BACKENDS_MOCK_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/status.hpp"
#include "castle_ext/io/rpmsg/backend.hpp"
#include "castle_ext/io/rpmsg/message.hpp"
#include "castle_ext/io/rpmsg/name_service.hpp"
#include "castle_ext/io/rpmsg/types.hpp"

#include <stdint.h>
#include <string.h>

namespace castle
{
namespace io
{
namespace rpmsg
{
namespace mock
{

template <size_type MaxEndpoints = 8U, size_type MaxPayload = default_max_payload_size>
class basic_backend
{
    struct endpoint_slot
    {
        bool active;
        char name[rpmsg_name_size];
        address local;
        address remote;
        void* context;
        receive_dispatch receive;
        unbind_dispatch unbind;
        bool tx_buffer_active;
        uint8_t tx_storage[MaxPayload];

        endpoint_slot() CASTLE_NOEXCEPT
            : active(false)
            , name{}
            , local(rpmsg_addr_any)
            , remote(rpmsg_addr_any)
            , context(nullptr)
            , receive(nullptr)
            , unbind(nullptr)
            , tx_buffer_active(false)
            , tx_storage{}
        {
        }
    };

public:
    struct sent_packet
    {
        address source;
        address destination;
        size_type size;
        uint8_t data[MaxPayload];

        sent_packet() CASTLE_NOEXCEPT
            : source(rpmsg_addr_any)
            , destination(rpmsg_addr_any)
            , size(0U)
            , data{}
        {
        }
    };

    basic_backend() CASTLE_NOEXCEPT
        : initialized_(false)
        , next_address_(reserved_address_count)
        , ns_context_(nullptr)
        , ns_callback_(nullptr)
        , sends_(0U)
        , forced_send_status_(castle::status::ok)
        , last_packet_()
    {
    }

    castle::status initialize(device_config CASTLE_CONST&, void* context,
                              name_service_dispatch callback) CASTLE_NOEXCEPT
    {
        if (initialized_)
        {
            return castle::status::invalid_config;
        }
        initialized_ = true;
        ns_context_ = context;
        ns_callback_ = callback;
        next_address_ = reserved_address_count;
        sends_ = 0U;
        forced_send_status_ = castle::status::ok;
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            endpoints_[i] = endpoint_slot();
        }
        return castle::status::ok;
    }

    castle::status shutdown() CASTLE_NOEXCEPT
    {
        if (!initialized_)
        {
            return castle::status::not_configured;
        }
        initialized_ = false;
        ns_context_ = nullptr;
        ns_callback_ = nullptr;
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            endpoints_[i] = endpoint_slot();
        }
        return castle::status::ok;
    }

    bool is_initialized() CASTLE_CONST CASTLE_NOEXCEPT { return initialized_; }

    castle::status create_endpoint(char CASTLE_CONST* name, address src, address dst,
                                   void* context, receive_dispatch receive,
                                   unbind_dispatch unbind, void*& handle,
                                   address& actual_source) CASTLE_NOEXCEPT
    {
        handle = nullptr;
        actual_source = rpmsg_addr_any;
        if (!initialized_)
        {
            return castle::status::not_configured;
        }
        if (name == nullptr || name[0] == '\0' || receive == nullptr
            || !is_valid_local_address(src)
            || (dst != rpmsg_addr_any && !is_valid_remote_address(dst)))
        {
            return castle::status::invalid_argument;
        }
        size_type name_length = 0U;
        while (name[name_length] != '\0' && name_length < rpmsg_name_size)
        {
            ++name_length;
        }
        if (name_length == 0U || name_length >= rpmsg_name_size)
        {
            return castle::status::out_of_range;
        }
        size_type index = MaxEndpoints;
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            if (!endpoints_[i].active)
            {
                index = i;
                break;
            }
        }
        if (index == MaxEndpoints)
        {
            return castle::status::full;
        }
        endpoint_slot& slot = endpoints_[index];
        slot = endpoint_slot();
        slot.active = true;
        memcpy(slot.name, name, name_length);
        slot.local = src == rpmsg_addr_any ? next_address_++ : src;
        slot.remote = dst;
        slot.context = context;
        slot.receive = receive;
        slot.unbind = unbind;
        actual_source = slot.local;
        handle = &slot;
        return castle::status::ok;
    }

    void destroy_endpoint(void* handle) CASTLE_NOEXCEPT
    {
        endpoint_slot* slot = find_slot(handle);
        if (slot != nullptr)
        {
            *slot = endpoint_slot();
        }
    }

    castle::status send(void* handle, address src, address dst,
                        void CASTLE_CONST* data, size_type length,
                        timeout_type timeout) CASTLE_NOEXCEPT
    {
        (void)timeout;
        if (!initialized_ || find_slot(handle) == nullptr)
        {
            return castle::status::not_configured;
        }
        if ((data == nullptr && length != 0U) || !is_valid_local_address(src)
            || !is_valid_remote_address(dst))
        {
            return castle::status::invalid_argument;
        }
        if (length > MaxPayload)
        {
            return castle::status::out_of_range;
        }
        if (forced_send_status_ != castle::status::ok)
        {
            castle::status result = forced_send_status_;
            forced_send_status_ = castle::status::ok;
            return result;
        }
        last_packet_.source = src;
        last_packet_.destination = dst;
        last_packet_.size = length;
        if (length != 0U)
        {
            memcpy(last_packet_.data, data, length);
        }
        ++sends_;
        return castle::status::ok;
    }

    castle::status acquire_tx_buffer(void* handle, bool wait,
                                     void*& data, size_type& capacity) CASTLE_NOEXCEPT
    {
        (void)wait;
        data = nullptr;
        capacity = 0U;
        endpoint_slot* slot = find_slot(handle);
        if (!initialized_ || slot == nullptr)
        {
            return castle::status::not_configured;
        }
        if (slot->tx_buffer_active)
        {
            return castle::status::full;
        }
        slot->tx_buffer_active = true;
        data = slot->tx_storage;
        capacity = MaxPayload;
        return castle::status::ok;
    }

    castle::status send_tx_buffer(void* handle, address src, address dst,
                                  void* data, size_type length) CASTLE_NOEXCEPT
    {
        endpoint_slot* slot = find_slot(handle);
        if (slot == nullptr || !slot->tx_buffer_active || data != slot->tx_storage)
        {
            return castle::status::invalid_argument;
        }
        castle::status result = send(handle, src, dst, data, length, no_wait);
        if (castle::succeeded(result))
        {
            slot->tx_buffer_active = false;
        }
        return result;
    }

    castle::status release_tx_buffer(void* handle, void* data) CASTLE_NOEXCEPT
    {
        endpoint_slot* slot = find_slot(handle);
        if (slot == nullptr || !slot->tx_buffer_active || data != slot->tx_storage)
        {
            return castle::status::invalid_argument;
        }
        slot->tx_buffer_active = false;
        return castle::status::ok;
    }

    address remote_address(void* handle) CASTLE_NOEXCEPT
    {
        endpoint_slot* slot = find_slot(handle);
        return slot == nullptr ? rpmsg_addr_any : slot->remote;
    }

    castle::status poll() CASTLE_NOEXCEPT
    {
        return initialized_ ? castle::status::ok : castle::status::not_configured;
    }

    /** @brief Injects an RX payload into the endpoint whose local address matches `destination`. */
    castle::status inject(address destination, address source,
                          void CASTLE_CONST* data, size_type length) CASTLE_NOEXCEPT
    {
        if (!initialized_)
        {
            return castle::status::not_configured;
        }
        if ((data == nullptr && length != 0U) || length > MaxPayload)
        {
            return castle::status::invalid_argument;
        }
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            endpoint_slot& slot = endpoints_[i];
            if (slot.active && slot.local == destination)
            {
                message_view message(source, destination, data, length);
                return slot.receive == nullptr ? castle::status::invalid_callback
                                               : slot.receive(slot.context, message);
            }
        }
        return castle::status::not_found;
    }

    /** @brief Simulates OpenAMP's name/address matching for one remote CREATE announcement. */
    castle::status announce(char CASTLE_CONST* name, address remote) CASTLE_NOEXCEPT
    {
        if (!initialized_)
        {
            return castle::status::not_configured;
        }
        if (name == nullptr || name[0] == '\0' || !is_valid_remote_address(remote))
        {
            return castle::status::invalid_argument;
        }
        size_type name_length = 0U;
        while (name[name_length] != '\0' && name_length < rpmsg_name_size)
        {
            ++name_length;
        }
        if (name_length == 0U || name_length >= rpmsg_name_size)
        {
            return castle::status::out_of_range;
        }

        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            endpoint_slot& slot = endpoints_[i];
            if (slot.active && strcmp(slot.name, name) == 0
                && (slot.remote == remote || slot.remote == rpmsg_addr_any))
            {
                slot.remote = remote;
                return castle::status::ok;
            }
        }

        if (ns_callback_ == nullptr)
        {
            return castle::status::not_configured;
        }
        name_service_event event;
        castle::status result = event.name.assign(name);
        if (!castle::succeeded(result) || event.name.empty())
        {
            return castle::status::invalid_argument;
        }
        event.endpoint_address = remote;
        event.action = name_service_action::create;
        ns_callback_(ns_context_, event);
        return castle::status::ok;
    }

    /** @brief Simulates the remote peer destroying the binding for one local endpoint. */
    castle::status unbind(address local) CASTLE_NOEXCEPT
    {
        if (!initialized_)
        {
            return castle::status::not_configured;
        }
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            endpoint_slot& slot = endpoints_[i];
            if (slot.active && slot.local == local)
            {
                if (slot.remote == rpmsg_addr_any)
                {
                    return castle::status::not_found;
                }
                address previous_remote = slot.remote;
                char service_name[rpmsg_name_size];
                memcpy(service_name, slot.name, sizeof(service_name));
                slot.remote = rpmsg_addr_any;
                if (slot.unbind != nullptr)
                {
                    slot.unbind(slot.context);
                }
                if (ns_callback_ != nullptr)
                {
                    name_service_event event;
                    if (event.name.assign(service_name) == castle::status::ok)
                    {
                        event.endpoint_address = previous_remote;
                        event.action = name_service_action::destroy;
                        ns_callback_(ns_context_, event);
                    }
                }
                return castle::status::ok;
            }
        }
        return castle::status::not_found;
    }

    /** @brief Changes the mock remote address to emulate an OpenAMP NS match. */
    castle::status bind(address local, address remote) CASTLE_NOEXCEPT
    {
        if (!initialized_)
        {
            return castle::status::not_configured;
        }
        if (!is_valid_remote_address(remote))
        {
            return castle::status::invalid_argument;
        }
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            if (endpoints_[i].active && endpoints_[i].local == local)
            {
                endpoints_[i].remote = remote;
                return castle::status::ok;
            }
        }
        return castle::status::not_found;
    }

    void fail_next_send(castle::status failure) CASTLE_NOEXCEPT { forced_send_status_ = failure; }
    size_type send_count() CASTLE_CONST CASTLE_NOEXCEPT { return sends_; }
    sent_packet CASTLE_CONST& last_packet() CASTLE_CONST CASTLE_NOEXCEPT { return last_packet_; }

private:
    endpoint_slot* find_slot(void* handle) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            if (endpoints_[i].active && handle == &endpoints_[i])
            {
                return &endpoints_[i];
            }
        }
        return nullptr;
    }

    bool initialized_;
    address next_address_;
    void* ns_context_;
    name_service_dispatch ns_callback_;
    size_type sends_;
    castle::status forced_send_status_;
    sent_packet last_packet_;
    endpoint_slot endpoints_[MaxEndpoints];
};

using backend = basic_backend<>;

} // namespace mock
} // namespace rpmsg
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_RPMSG_BACKENDS_MOCK_HPP
