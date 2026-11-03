// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file types.hpp
 * @brief Platform-neutral RPMsg constants, configuration and fixed-capacity options.
 *
 * The default payload is 496 bytes: the common OpenAMP RPMsg buffer size is
 * 512 bytes, of which 16 bytes are reserved for the RPMsg transport header.
 */
#ifndef CASTLE_EXT_IO_RPMSG_TYPES_HPP
#define CASTLE_EXT_IO_RPMSG_TYPES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace io
{
namespace rpmsg
{

using address = uint32_t;
using timeout_type = int32_t;

static CASTLE_INLINE CASTLE_CONSTEXPR address rpmsg_addr_any = 0xFFFFFFFFU;
static CASTLE_INLINE CASTLE_CONSTEXPR address rpmsg_ns_ept_addr = 0x00000035U;
static CASTLE_INLINE CASTLE_CONSTEXPR address reserved_address_count = 1024U;
static CASTLE_INLINE CASTLE_CONSTEXPR size_type rpmsg_name_size = 32U;
static CASTLE_INLINE CASTLE_CONSTEXPR size_type max_name_length = rpmsg_name_size - 1U;
static CASTLE_INLINE CASTLE_CONSTEXPR size_type rpmsg_header_size = 16U;
static CASTLE_INLINE CASTLE_CONSTEXPR size_type default_buffer_size = 512U;
static CASTLE_INLINE CASTLE_CONSTEXPR size_type default_max_payload_size = default_buffer_size - rpmsg_header_size;
static CASTLE_INLINE CASTLE_CONSTEXPR timeout_type no_wait = 0;
static CASTLE_INLINE CASTLE_CONSTEXPR timeout_type wait_forever = -1;

/** @brief Local VirtIO device role. The peer must use the complementary role. */
enum class role : uint8_t
{
    host = 0U,
    remote = 1U
};

/** @brief RPMsg Name Service operation encoded in `rpmsg_ns_msg::flags`. */
enum class name_service_action : uint32_t
{
    create = 0U,
    destroy = 1U
};

/**
 * @brief Compile-time limits shared by the RPMsg device, endpoint table and backend.
 *
 * `MaxNameLength` is the maximum number of visible characters, excluding the
 * trailing NUL in the 32-byte OpenAMP service-name field.
 */
template <size_type MaxEndpoints = 8U,
          size_type MaxChannels = 8U,
          size_type MaxNameLength = max_name_length,
          size_type MaxPayloadSize = default_max_payload_size,
          size_type BufferCount = 16U,
          size_type BufferSize = default_buffer_size,
          size_type CallbackStorage = 48U,
          bool AutoBind = true,
          bool ZeroCopy = true,
          role DefaultRole = role::host>
struct options
{
    static_assert(MaxEndpoints > 0U, "RPMsg requires at least one endpoint slot");
    static_assert(MaxChannels > 0U, "RPMsg requires at least one channel slot");
    static_assert(MaxNameLength > 0U && MaxNameLength <= ::castle::io::rpmsg::max_name_length,
                  "RPMsg name length must fit in the 32-byte RPMsg name field");
    static_assert(MaxPayloadSize > 0U && MaxPayloadSize <= 0xFFFFU,
                  "RPMsg payload length must fit in the 16-bit RPMsg header length");
    static_assert(BufferCount > 0U, "RPMsg shared buffer count must be positive");
    static_assert(BufferSize >= rpmsg_header_size + MaxPayloadSize,
                  "RPMsg buffer size must include the 16-byte header and configured payload");
    static_assert(CallbackStorage >= sizeof(void*), "Callback storage is too small");
    static_assert(DefaultRole == role::host || DefaultRole == role::remote,
                  "Invalid default RPMsg role");

    static CASTLE_CONSTEXPR size_type max_endpoints = MaxEndpoints;
    static CASTLE_CONSTEXPR size_type max_channels = MaxChannels;
    static CASTLE_CONSTEXPR size_type max_name_length = MaxNameLength;
    static CASTLE_CONSTEXPR size_type max_payload_size = MaxPayloadSize;
    static CASTLE_CONSTEXPR size_type buffer_count = BufferCount;
    static CASTLE_CONSTEXPR size_type buffer_size = BufferSize;
    static CASTLE_CONSTEXPR size_type callback_storage = CallbackStorage;
    static CASTLE_CONSTEXPR bool auto_bind = AutoBind;
    static CASTLE_CONSTEXPR bool zero_copy = ZeroCopy;
    static CASTLE_CONSTEXPR role default_role = DefaultRole;
};

/** @brief Runtime platform memory/role description passed to the backend at initialization. */
struct device_config
{
    role node_role;
    void* shared_memory;
    size_type shared_memory_size;
    size_type buffer_count;
    size_type buffer_size;

    CASTLE_CONSTEXPR explicit device_config(role selected_role = role::host,
                                            void* memory = nullptr,
                                            size_type memory_size = 0U,
                                            size_type count = 16U,
                                            size_type size = default_buffer_size) CASTLE_NOEXCEPT
        : node_role(selected_role)
        , shared_memory(memory)
        , shared_memory_size(memory_size)
        , buffer_count(count)
        , buffer_size(size)
    {
    }
};

/** @brief Validates role and shared-memory pointer/size consistency. */
CASTLE_NODISCARD CASTLE_INLINE castle::status validate_config(device_config CASTLE_CONST& config) CASTLE_NOEXCEPT
{
    if ((config.node_role != role::host && config.node_role != role::remote)
        || ((config.shared_memory == nullptr) != (config.shared_memory_size == 0U))
        || config.buffer_count == 0U
        || config.buffer_size < rpmsg_header_size)
    {
        return castle::status::invalid_config;
    }
    return castle::status::ok;
}

/** @brief True when the address is in the RPMsg pre-reserved range (excluding ADDR_ANY). */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_reserved_address(address value) CASTLE_NOEXCEPT
{
    return value != rpmsg_addr_any && value < reserved_address_count;
}

/** @brief True when an explicit local source address is usable by CASTLE. */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_valid_local_address(address value) CASTLE_NOEXCEPT
{
    return value == rpmsg_addr_any || value >= reserved_address_count;
}

/** @brief True when a remote service address is usable by CASTLE. */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_valid_remote_address(address value) CASTLE_NOEXCEPT
{
    return value != rpmsg_addr_any && !is_reserved_address(value);
}

/** @brief True when `timeout` is the `wait_forever` sentinel. */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_forever(timeout_type timeout) CASTLE_NOEXCEPT
{
    return timeout == wait_forever;
}

} // namespace rpmsg
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_RPMSG_TYPES_HPP
