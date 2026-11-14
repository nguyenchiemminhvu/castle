// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file endpoint.hpp
 * @brief Move-light endpoint handle for one fixed slot owned by `basic_device`.
 */
#ifndef CASTLE_EXT_IO_RPMSG_ENDPOINT_HPP
#define CASTLE_EXT_IO_RPMSG_ENDPOINT_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/status.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/forward.hpp"
#include "castle_ext/io/rpmsg/message.hpp"
#include "castle_ext/io/rpmsg/types.hpp"

namespace castle
{
namespace io
{
namespace rpmsg
{

template <typename Backend, typename Options>
class basic_device;

template <typename Backend, typename Options>
class basic_endpoint
{
public:
    using device_type = basic_device<Backend, Options>;
    using receive_callback = castle::callbacks::function<void(message_view CASTLE_CONST&), Options::callback_storage>;
    using unbind_callback = castle::callbacks::function<void(), Options::callback_storage>;

    basic_endpoint() CASTLE_NOEXCEPT
        : device_(nullptr)
        , slot_(0U)
        , generation_(0U)
    {
    }

    basic_endpoint(basic_endpoint CASTLE_CONST&) CASTLE_DELETE;
    basic_endpoint& operator=(basic_endpoint CASTLE_CONST&) CASTLE_DELETE;

    basic_endpoint(basic_endpoint&& other) CASTLE_NOEXCEPT
        : device_(other.device_)
        , slot_(other.slot_)
        , generation_(other.generation_)
    {
        other.reset();
    }

    basic_endpoint& operator=(basic_endpoint&& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            if (device_ != nullptr)
            {
                (void)destroy();
            }
            device_ = other.device_;
            slot_ = other.slot_;
            generation_ = other.generation_;
            other.reset();
        }
        return *this;
    }

    ~basic_endpoint()
    {
        if (device_ != nullptr)
        {
            (void)destroy();
        }
    }

    /** @brief Creates an endpoint and binds its RX callback to the requested service. */
    template <typename ReceiveCallable, typename UnbindCallable = decltype(nullptr)>
    castle::status create(device_type& device,
                          char CASTLE_CONST* service_name,
                          address local_address,
                          address remote_address,
                          ReceiveCallable&& on_receive,
                          UnbindCallable&& on_unbind = nullptr) CASTLE_NOEXCEPT
    {
        if (device_ != nullptr)
        {
            return castle::status::already_exists;
        }
        receive_callback receive(CASTLE_FORWARD<ReceiveCallable>(on_receive));
        unbind_callback unbind(CASTLE_FORWARD<UnbindCallable>(on_unbind));
        return device.attach_endpoint(*this, service_name, local_address, remote_address,
                                      CASTLE_MOVE(receive), CASTLE_MOVE(unbind));
    }

    /** @brief Destroys the endpoint and unregisters its local address. */
    castle::status destroy() CASTLE_NOEXCEPT
    {
        if (device_ == nullptr)
        {
            return castle::status::not_configured;
        }
        return device_->detach_endpoint(*this);
    }

    /** @brief True while this handle still refers to an active device slot. */
    CASTLE_NODISCARD bool is_created() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return device_ != nullptr && device_->is_endpoint_valid(*this);
    }

    /** @brief Returns the local source address, or `RPMSG_ADDR_ANY` for an invalid handle. */
    CASTLE_NODISCARD address source_address() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_created() ? device_->endpoint_source(*this) : rpmsg_addr_any;
    }

    /** @brief Returns the current remote destination, including Name-Service binding updates. */
    CASTLE_NODISCARD address destination_address() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_created() ? device_->endpoint_destination(*this) : rpmsg_addr_any;
    }

    /** @brief Sends to the endpoint's current remote destination, waiting according to `timeout`. */
    castle::status send(void CASTLE_CONST* data, size_type length,
                        timeout_type timeout = wait_forever) CASTLE_NOEXCEPT
    {
        if (!is_created())
        {
            return castle::status::not_configured;
        }
        return device_->send_endpoint(*this, source_address(), destination_address(), data, length, timeout);
    }

    /** @brief Non-blocking send to the endpoint's current remote destination. */
    castle::status try_send(void CASTLE_CONST* data, size_type length) CASTLE_NOEXCEPT
    {
        return send(data, length, no_wait);
    }

    /** @brief Sends to an explicitly selected remote endpoint address. */
    castle::status send_to(void CASTLE_CONST* data, size_type length, address remote,
                           timeout_type timeout = wait_forever) CASTLE_NOEXCEPT
    {
        if (!is_created())
        {
            return castle::status::not_configured;
        }
        return device_->send_endpoint(*this, source_address(), remote, data, length, timeout);
    }

    /** @brief Non-blocking send to an explicitly selected remote endpoint address. */
    castle::status try_send_to(void CASTLE_CONST* data, size_type length, address remote) CASTLE_NOEXCEPT
    {
        return send_to(data, length, remote, no_wait);
    }

    /** @brief Alias for `send_to`, matching the RPMsg `sendto` naming convention. */
    castle::status sendto(void CASTLE_CONST* data, size_type length, address remote,
                          timeout_type timeout = wait_forever) CASTLE_NOEXCEPT
    {
        return send_to(data, length, remote, timeout);
    }

    /** @brief Non-blocking `sendto` alias. */
    castle::status try_sendto(void CASTLE_CONST* data, size_type length, address remote) CASTLE_NOEXCEPT
    {
        return try_send_to(data, length, remote);
    }

    /** @brief Sends using explicitly selected source and destination addresses. */
    castle::status send_offchannel(void CASTLE_CONST* data, size_type length,
                                   address local, address remote,
                                   timeout_type timeout = wait_forever) CASTLE_NOEXCEPT
    {
        if (!is_created())
        {
            return castle::status::not_configured;
        }
        return device_->send_endpoint(*this, local, remote, data, length, timeout);
    }

    /** @brief Non-blocking off-channel send with explicit source and destination addresses. */
    castle::status try_send_offchannel(void CASTLE_CONST* data, size_type length,
                                       address local, address remote) CASTLE_NOEXCEPT
    {
        return send_offchannel(data, length, local, remote, no_wait);
    }

    /** @brief Acquires a backend TX payload buffer for zero-copy construction. */
    castle::status acquire_tx_buffer(tx_buffer& out, bool wait = false) CASTLE_NOEXCEPT
    {
        if (!is_created())
        {
            out.reset();
            return castle::status::not_configured;
        }
        return device_->acquire_tx_buffer(*this, out, wait);
    }

    /** @brief Sends a previously acquired TX buffer without copying its payload. */
    castle::status send_nocopy(tx_buffer& buffer, size_type length,
                               address remote = rpmsg_addr_any) CASTLE_NOEXCEPT
    {
        if (!is_created())
        {
            return castle::status::not_configured;
        }
        return device_->send_nocopy(*this, buffer, length, remote);
    }

    /** @brief Releases an acquired TX buffer without sending it. */
    castle::status release_tx_buffer(tx_buffer& buffer) CASTLE_NOEXCEPT
    {
        if (!is_created())
        {
            return castle::status::not_configured;
        }
        return device_->release_tx_buffer(*this, buffer);
    }

private:
    friend class basic_device<Backend, Options>;

    void reset() CASTLE_NOEXCEPT
    {
        device_ = nullptr;
        slot_ = 0U;
        generation_ = 0U;
    }

    device_type* device_;
    size_type slot_;
    uint32_t generation_;
};

template <typename Backend, typename Options = options<> >
using endpoint = basic_endpoint<Backend, Options>;

} // namespace rpmsg
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_RPMSG_ENDPOINT_HPP
