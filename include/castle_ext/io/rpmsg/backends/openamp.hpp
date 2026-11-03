// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file openamp.hpp
 * @brief Optional OpenAMP RPMsg/VirtIO backend for Linux userspace and Zephyr integrations.
 *
 * Define `CASTLE_RPMSG_ENABLE_OPENAMP` and provide OpenAMP/libmetal include paths
 * to enable this adapter. The platform must first prepare its resource table,
 * VirtIO device, shared-memory mapping and mailbox/IPI plumbing. This adapter
 * initializes the RPMsg shared-buffer pool and `rpmsg_virtio` device, then
 * adapts the standard OpenAMP endpoint/send APIs to CASTLE's compile-time API.
 */
#ifndef CASTLE_EXT_IO_RPMSG_BACKENDS_OPENAMP_HPP
#define CASTLE_EXT_IO_RPMSG_BACKENDS_OPENAMP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/io/rpmsg/backend.hpp"
#include "castle_ext/io/rpmsg/types.hpp"

#if defined(CASTLE_RPMSG_ENABLE_OPENAMP)

#include <openamp/rpmsg.h>
#include <openamp/rpmsg_virtio.h>
#include <openamp/virtio.h>
#include <metal/io.h>

#include <errno.h>
#include <stdint.h>
#include <string.h>

namespace castle
{
namespace io
{
namespace rpmsg
{
namespace backends
{

/** @brief Caller-owned OpenAMP objects prepared by platform-specific remoteproc/Zephyr glue. */
struct openamp_resources
{
    struct rpmsg_virtio_device* rpmsg_virtio;
    struct virtio_device* virtio;
    struct metal_io_region* shared_memory_io;

    openamp_resources(struct rpmsg_virtio_device* rvdev = nullptr,
                      struct virtio_device* vdev = nullptr,
                      struct metal_io_region* shm_io = nullptr) CASTLE_NOEXCEPT
        : rpmsg_virtio(rvdev)
        , virtio(vdev)
        , shared_memory_io(shm_io)
    {
    }
};

/**
 * @brief Fixed-capacity OpenAMP adapter; one instance owns at most MaxEndpoints native endpoints.
 * @tparam MaxEndpoints Must match or exceed the CASTLE device endpoint capacity.
 */
template <size_type MaxEndpoints = 8U>
class openamp_backend
{
    static_assert(MaxEndpoints > 0U, "OpenAMP backend requires endpoint storage");

    struct endpoint_binding
    {
        void* context;
        receive_dispatch receive;
        unbind_dispatch unbind;

        endpoint_binding() CASTLE_NOEXCEPT
            : context(nullptr)
            , receive(nullptr)
            , unbind(nullptr)
        {
        }
    };

    struct endpoint_slot
    {
        bool active;
        struct rpmsg_endpoint native_endpoint;
        endpoint_binding binding;

        endpoint_slot() CASTLE_NOEXCEPT
            : active(false)
            , native_endpoint{}
            , binding()
        {
        }
    };

public:
    explicit openamp_backend(openamp_resources resources) CASTLE_NOEXCEPT
        : resources_(resources)
        , shared_pool_()
        , rpmsg_device_(nullptr)
        , ns_context_(nullptr)
        , ns_callback_(nullptr)
        , initialized_(false)
        , registry_index_(registry_capacity)
    {
    }

    openamp_backend(openamp_backend CASTLE_CONST&) CASTLE_DELETE;
    openamp_backend& operator=(openamp_backend CASTLE_CONST&) CASTLE_DELETE;

    ~openamp_backend()
    {
        if (initialized_)
        {
            (void)shutdown();
        }
    }

    /**
     * @brief Initializes the OpenAMP pool and RPMsg VirtIO device around preconfigured platform resources.
     *
     * `config.shared_memory` is the RPMsg buffer pool and is required only for `role::host`; a
     * `role::remote` device consumes buffers provided by the host through the vrings. For the remote
     * role this call blocks inside `rpmsg_init_vdev()` until the host sets driver-ready.
     * @return `ok`; `invalid_config` for null resources, role mismatch or a host without a pool;
     *         `full` when four backends are already registered; otherwise the mapped OpenAMP error.
     */
    castle::status initialize(device_config CASTLE_CONST& config, void* ns_context,
                              name_service_dispatch ns_callback) CASTLE_NOEXCEPT
    {
        if (initialized_ || (config.node_role != role::host && config.node_role != role::remote)
            || resources_.rpmsg_virtio == nullptr || resources_.virtio == nullptr
            || resources_.shared_memory_io == nullptr
            || (config.node_role == role::host
                && (config.shared_memory == nullptr || config.shared_memory_size == 0U)))
        {
            return castle::status::invalid_config;
        }
        unsigned int expected_role = config.node_role == role::host ? RPMSG_HOST : RPMSG_REMOTE;
        // rpmsg_virtio_get_role() would dereference rvdev->vdev, which is only set by rpmsg_init_vdev().
        if (resources_.virtio->role != expected_role)
        {
            return castle::status::invalid_config;
        }
        registry_index_ = reserve_registry_slot();
        if (registry_index_ == registry_capacity)
        {
            return castle::status::full;
        }
        registry_[registry_index_] = this;
        ns_context_ = ns_context;
        ns_callback_ = ns_callback;

        if (config.shared_memory != nullptr)
        {
            rpmsg_virtio_init_shm_pool(&shared_pool_, config.shared_memory, config.shared_memory_size);
        }
        int result = rpmsg_init_vdev(resources_.rpmsg_virtio, resources_.virtio,
                                     &ns_bind_trampoline, resources_.shared_memory_io, &shared_pool_);
        if (result != RPMSG_SUCCESS)
        {
            registry_[registry_index_] = nullptr;
            registry_index_ = registry_capacity;
            ns_context_ = nullptr;
            ns_callback_ = nullptr;
            return map_error(result);
        }
        rpmsg_device_ = rpmsg_virtio_get_rpmsg_device(resources_.rpmsg_virtio);
        if (rpmsg_device_ == nullptr)
        {
            rpmsg_deinit_vdev(resources_.rpmsg_virtio);
            registry_[registry_index_] = nullptr;
            registry_index_ = registry_capacity;
            ns_context_ = nullptr;
            ns_callback_ = nullptr;
            return castle::status::invalid_config;
        }
        rpmsg_device_->ns_unbind_cb = &ns_unbind_trampoline;
        initialized_ = true;
        return castle::status::ok;
    }

    /** @brief Destroys all native endpoints and deinitializes the RPMsg VirtIO device. */
    castle::status shutdown() CASTLE_NOEXCEPT
    {
        if (!initialized_)
        {
            return castle::status::not_configured;
        }
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            if (endpoints_[i].active)
            {
                rpmsg_destroy_ept(&endpoints_[i].native_endpoint);
                endpoints_[i].active = false;
                endpoints_[i].binding = endpoint_binding();
            }
        }
        if (rpmsg_device_ != nullptr)
        {
            rpmsg_device_->ns_unbind_cb = nullptr;
        }
        rpmsg_deinit_vdev(resources_.rpmsg_virtio);
        rpmsg_device_ = nullptr;
        initialized_ = false;
        ns_context_ = nullptr;
        ns_callback_ = nullptr;
        if (registry_index_ < registry_capacity)
        {
            registry_[registry_index_] = nullptr;
            registry_index_ = registry_capacity;
        }
        return castle::status::ok;
    }

    CASTLE_NODISCARD bool is_initialized() CASTLE_CONST CASTLE_NOEXCEPT { return initialized_; }

    /** @brief Creates a native OpenAMP endpoint and installs CASTLE callback trampolines. */
    castle::status create_endpoint(char CASTLE_CONST* name, address src, address dst,
                                   void* context, receive_dispatch receive,
                                   unbind_dispatch unbind, void*& handle,
                                   address& actual_source) CASTLE_NOEXCEPT
    {
        handle = nullptr;
        actual_source = rpmsg_addr_any;
        if (!initialized_ || rpmsg_device_ == nullptr)
        {
            return castle::status::not_configured;
        }
        if (name == nullptr || name[0] == '\0' || receive == nullptr
            || !is_valid_local_address(src)
            || (dst != rpmsg_addr_any && !is_valid_remote_address(dst)))
        {
            return castle::status::invalid_argument;
        }
        size_type slot_index = MaxEndpoints;
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            if (!endpoints_[i].active)
            {
                slot_index = i;
                break;
            }
        }
        if (slot_index == MaxEndpoints)
        {
            return castle::status::full;
        }

        endpoint_slot& slot = endpoints_[slot_index];
        memset(&slot.native_endpoint, 0, sizeof(slot.native_endpoint));
        slot.binding.context = context;
        slot.binding.receive = receive;
        slot.binding.unbind = unbind;
        slot.native_endpoint.priv = &slot.binding;
        int result = rpmsg_create_ept(&slot.native_endpoint, rpmsg_device_, name,
                                      src, dst, &receive_trampoline, &unbind_trampoline);
        if (result != RPMSG_SUCCESS)
        {
            slot.binding = endpoint_binding();
            return result == RPMSG_ERR_ADDR && src != rpmsg_addr_any
                 ? castle::status::already_exists : map_error(result);
        }
        slot.active = true;
        handle = &slot.native_endpoint;
        actual_source = slot.native_endpoint.addr;
        return castle::status::ok;
    }

    /** @brief Destroys one native OpenAMP endpoint. */
    void destroy_endpoint(void* handle) CASTLE_NOEXCEPT
    {
        endpoint_slot* slot = find_slot(handle);
        if (slot != nullptr)
        {
            rpmsg_destroy_ept(&slot->native_endpoint);
            slot->active = false;
            slot->binding = endpoint_binding();
        }
    }

    /**
     * @brief Sends one message; `no_wait` uses `rpmsg_trysend_offchannel`, `wait_forever` uses OpenAMP's
     *        bounded blocking wait (about 15 s by default).
     * @return `ok`; `full` when no TX buffer is free in `no_wait` mode; `time_out` when the wait expires;
     *         `data_loss` when the peer's buffers are smaller than `length` and OpenAMP truncated the
     *         payload; `invalid_config` for any other timeout value.
     */
    castle::status send(void* handle, address src, address dst,
                        void CASTLE_CONST* data, size_type length,
                        timeout_type timeout) CASTLE_NOEXCEPT
    {
        struct rpmsg_endpoint* ept = static_cast<struct rpmsg_endpoint*>(handle);
        if (!initialized_ || find_slot(handle) == nullptr)
        {
            return castle::status::not_configured;
        }
        if ((data == nullptr && length != 0U) || length > static_cast<size_type>(0x7FFFFFFFU)
            || !is_valid_local_address(src) || !is_valid_remote_address(dst))
        {
            return castle::status::invalid_argument;
        }
        if (timeout != no_wait && timeout != wait_forever)
        {
            return castle::status::invalid_config;
        }
        int result = timeout == no_wait
                   ? rpmsg_trysend_offchannel(ept, src, dst, data, static_cast<int>(length))
                   : rpmsg_send_offchannel(ept, src, dst, data, static_cast<int>(length));
        if (result == RPMSG_ERR_NO_BUFF && timeout == wait_forever)
        {
            return castle::status::time_out;
        }
        if (result < 0)
        {
            return map_error(result);
        }
        // OpenAMP silently clamps the length to the TX buffer size and returns the clamped count.
        return static_cast<size_type>(result) == length ? castle::status::ok : castle::status::data_loss;
    }

    /** @brief Acquires one OpenAMP shared-memory TX payload buffer. */
    castle::status acquire_tx_buffer(void* handle, bool wait,
                                     void*& data, size_type& capacity) CASTLE_NOEXCEPT
    {
        data = nullptr;
        capacity = 0U;
        struct rpmsg_endpoint* ept = static_cast<struct rpmsg_endpoint*>(handle);
        if (!initialized_ || find_slot(handle) == nullptr)
        {
            return castle::status::not_configured;
        }
        uint32_t native_capacity = 0U;
        data = rpmsg_get_tx_payload_buffer(ept, &native_capacity, wait ? 1 : 0);
        capacity = native_capacity;
        return data == nullptr ? (wait ? castle::status::time_out : castle::status::full)
                               : castle::status::ok;
    }

    /** @brief Commits a buffer obtained from `rpmsg_get_tx_payload_buffer()` without copying. */
    castle::status send_tx_buffer(void* handle, address src, address dst,
                                  void* data, size_type length) CASTLE_NOEXCEPT
    {
        struct rpmsg_endpoint* ept = static_cast<struct rpmsg_endpoint*>(handle);
        if (!initialized_ || find_slot(handle) == nullptr || data == nullptr)
        {
            return castle::status::invalid_argument;
        }
        if (!is_valid_local_address(src) || !is_valid_remote_address(dst))
        {
            return castle::status::invalid_argument;
        }
        if (length > static_cast<size_type>(0x7FFFFFFFU))
        {
            return castle::status::out_of_range;
        }
        int result = rpmsg_send_offchannel_nocopy(ept, src, dst, data, static_cast<int>(length));
        return result < 0 ? map_error(result) : castle::status::ok;
    }

    /** @brief Returns an unused shared-memory TX buffer to OpenAMP. */
    castle::status release_tx_buffer(void* handle, void* data) CASTLE_NOEXCEPT
    {
        struct rpmsg_endpoint* ept = static_cast<struct rpmsg_endpoint*>(handle);
        if (!initialized_ || find_slot(handle) == nullptr || data == nullptr)
        {
            return castle::status::invalid_argument;
        }
        int result = rpmsg_release_tx_buffer(ept, data);
        return result < 0 ? map_error(result) : castle::status::ok;
    }

    /** @brief Reads the destination address, which OpenAMP updates when matching NS arrives. */
    address remote_address(void* handle) CASTLE_NOEXCEPT
    {
        struct rpmsg_endpoint* ept = static_cast<struct rpmsg_endpoint*>(handle);
        return (!initialized_ || find_slot(handle) == nullptr) ? rpmsg_addr_any : ept->dest_addr;
    }

    /** @brief OpenAMP notification processing is driven by the platform virtqueue/mailbox integration. */
    castle::status poll() CASTLE_NOEXCEPT
    {
        return initialized_ ? castle::status::ok : castle::status::not_configured;
    }

private:
    static CASTLE_CONST size_type registry_capacity = 4U;

    static size_type reserve_registry_slot() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < registry_capacity; ++i)
        {
            if (registry_[i] == nullptr)
            {
                return i;
            }
        }
        return registry_capacity;
    }

    endpoint_slot* find_slot(void* handle) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxEndpoints; ++i)
        {
            if (endpoints_[i].active && handle == &endpoints_[i].native_endpoint)
            {
                return &endpoints_[i];
            }
        }
        return nullptr;
    }

    static int receive_trampoline(struct rpmsg_endpoint* ept, void* data,
                                  size_t length, uint32_t src, void* priv) CASTLE_NOEXCEPT
    {
        // OpenAMP asserts on a negative callback status, so rejected frames are dropped, not reported.
        endpoint_binding* binding = static_cast<endpoint_binding*>(priv);
        if (ept != nullptr && binding != nullptr && binding->receive != nullptr)
        {
            message_view message(src, ept->addr, data, length);
            (void)binding->receive(binding->context, message);
        }
        return RPMSG_SUCCESS;
    }

    static void unbind_trampoline(struct rpmsg_endpoint* ept) CASTLE_NOEXCEPT
    {
        if (ept == nullptr)
        {
            return;
        }
        endpoint_binding* binding = static_cast<endpoint_binding*>(ept->priv);
        if (binding != nullptr && binding->unbind != nullptr)
        {
            binding->unbind(binding->context);
        }
    }

    static void ns_bind_trampoline(struct rpmsg_device* rdev,
                                   char CASTLE_CONST* name, uint32_t dest) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < registry_capacity; ++i)
        {
            openamp_backend* self = registry_[i];
            if (self != nullptr && self->initialized_ && self->rpmsg_device_ == rdev
                && self->ns_callback_ != nullptr && name != nullptr)
            {
                name_service_event event;
                if (event.name.assign(name) == castle::status::ok)
                {
                    event.endpoint_address = dest;
                    event.action = name_service_action::create;
                    self->ns_callback_(self->ns_context_, event);
                }
                return;
            }
        }
    }

    static void ns_unbind_trampoline(struct rpmsg_device* rdev,
                                     char CASTLE_CONST* name, uint32_t dest) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < registry_capacity; ++i)
        {
            openamp_backend* self = registry_[i];
            if (self != nullptr && self->initialized_ && self->rpmsg_device_ == rdev
                && self->ns_callback_ != nullptr && name != nullptr)
            {
                name_service_event event;
                if (event.name.assign(name) == castle::status::ok)
                {
                    event.endpoint_address = dest;
                    event.action = name_service_action::destroy;
                    self->ns_callback_(self->ns_context_, event);
                }
                return;
            }
        }
    }

    static castle::status map_error(int error) CASTLE_NOEXCEPT
    {
        if (error == RPMSG_ERR_NO_BUFF || error == RPMSG_ERR_NO_MEM)
        {
            return castle::status::full;
        }
        if (error == RPMSG_ERR_PARAM)
        {
            return castle::status::invalid_argument;
        }
        if (error == RPMSG_ERR_ADDR)
        {
            return castle::status::full;
        }
#ifdef RPMSG_ERR_BUFF_SIZE
        if (error == RPMSG_ERR_BUFF_SIZE)
        {
            return castle::status::out_of_range;
        }
#endif
#ifdef RPMSG_ERR_PERM
        if (error == RPMSG_ERR_PERM)
        {
            return castle::status::not_configured;
        }
#endif
#ifdef RPMSG_EOPNOTSUPP
        if (error == RPMSG_EOPNOTSUPP)
        {
            return castle::status::not_configured;
        }
#endif
        if (error == RPMSG_ERR_DEV_STATE || error == RPMSG_ERR_INIT)
        {
            return castle::status::not_configured;
        }
#ifdef ERESTARTSYS
        if (error == -ERESTARTSYS)
        {
            return castle::status::time_out;
        }
#endif
        if (error == -ETIMEDOUT)
        {
            return castle::status::time_out;
        }
        return castle::status::system_call_error;
    }

    openamp_resources resources_;
    struct rpmsg_virtio_shm_pool shared_pool_;
    struct rpmsg_device* rpmsg_device_;
    void* ns_context_;
    name_service_dispatch ns_callback_;
    bool initialized_;
    size_type registry_index_;
    endpoint_slot endpoints_[MaxEndpoints];
    CASTLE_INLINE static openamp_backend* registry_[registry_capacity] = {};
};

} // namespace backends
} // namespace rpmsg
} // namespace io
} // namespace castle

#endif // CASTLE_RPMSG_ENABLE_OPENAMP

#endif // CASTLE_EXT_IO_RPMSG_BACKENDS_OPENAMP_HPP
