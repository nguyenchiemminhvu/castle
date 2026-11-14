// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file name_service.hpp
 * @brief Fixed-layout RPMsg Name Service message parsing and event representation.
 *
 * The payload is the OpenAMP/Linux `rpmsg_ns_msg`: 32 name bytes, a 32-bit
 * endpoint address and a 32-bit flags value. Integer fields use the native
 * shared-memory representation expected by the selected OpenAMP implementation.
 */
#ifndef CASTLE_EXT_IO_RPMSG_NAME_SERVICE_HPP
#define CASTLE_EXT_IO_RPMSG_NAME_SERVICE_HPP

#include "castle/dsa/string.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/status.hpp"
#include "castle_ext/io/rpmsg/types.hpp"

#include <stdint.h>
#include <string.h>

namespace castle
{
namespace io
{
namespace rpmsg
{

static CASTLE_INLINE CASTLE_CONSTEXPR size_type name_service_message_size = rpmsg_name_size + 8U;

/** @brief Decoded RPMsg Name Service announcement. */
struct name_service_event
{
    dsa::string<max_name_length> name;
    address endpoint_address;
    name_service_action action;

    name_service_event() CASTLE_NOEXCEPT
        : name()
        , endpoint_address(rpmsg_addr_any)
        , action(name_service_action::create)
    {
    }
};

/**
 * @brief Decodes one packed `rpmsg_ns_msg` payload without alignment assumptions.
 * @param data Message payload; must point to at least 40 readable bytes.
 * @param length Payload size, which must equal 40 bytes.
 * @param out Decoded event, unchanged on failure.
 * @return `ok`; `invalid_argument` for null, wrong length, missing NUL or unknown flags.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::status decode_name_service_message(void CASTLE_CONST* data,
                                                                    size_type length,
                                                                    name_service_event& out) CASTLE_NOEXCEPT
{
    if (data == nullptr || length != name_service_message_size)
    {
        return castle::status::invalid_argument;
    }

    uint8_t CASTLE_CONST* bytes = static_cast<uint8_t CASTLE_CONST*>(data);
    size_type name_length = 0U;
    while (name_length < rpmsg_name_size && bytes[name_length] != 0U)
    {
        ++name_length;
    }
    if (name_length == 0U || name_length == rpmsg_name_size || name_length > max_name_length)
    {
        return castle::status::invalid_argument;
    }

    uint32_t raw_address = 0U;
    uint32_t raw_flags = 0U;
    memcpy(&raw_address, bytes + rpmsg_name_size, sizeof(raw_address));
    memcpy(&raw_flags, bytes + rpmsg_name_size + sizeof(raw_address), sizeof(raw_flags));

    if (!is_valid_remote_address(raw_address)
        || (raw_flags != static_cast<uint32_t>(name_service_action::create)
            && raw_flags != static_cast<uint32_t>(name_service_action::destroy)))
    {
        return castle::status::invalid_argument;
    }

    name_service_event candidate;
    if (candidate.name.assign(reinterpret_cast<char CASTLE_CONST*>(bytes)) != castle::status::ok)
    {
        return castle::status::invalid_argument;
    }
    candidate.endpoint_address = raw_address;
    candidate.action = static_cast<name_service_action>(raw_flags);
    out = candidate;
    return castle::status::ok;
}

/**
 * @brief Encodes a Name Service event into the exact 40-byte RPMsg payload.
 * @param event Source event.
 * @param output Output buffer, at least `name_service_message_size` bytes.
 * @param capacity Output buffer capacity.
 * @return `ok`; `invalid_argument` for invalid event; `out_of_range` when output is short.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::status encode_name_service_message(name_service_event CASTLE_CONST& event,
                                                                    void* output,
                                                                    size_type capacity) CASTLE_NOEXCEPT
{
    if (output == nullptr || event.name.empty() || !is_valid_remote_address(event.endpoint_address)
        || (event.action != name_service_action::create && event.action != name_service_action::destroy))
    {
        return castle::status::invalid_argument;
    }
    if (capacity < name_service_message_size)
    {
        return castle::status::out_of_range;
    }

    uint8_t* bytes = static_cast<uint8_t*>(output);
    memset(bytes, 0, name_service_message_size);
    memcpy(bytes, event.name.data(), event.name.size());
    uint32_t raw_address = event.endpoint_address;
    uint32_t raw_flags = static_cast<uint32_t>(event.action);
    memcpy(bytes + rpmsg_name_size, &raw_address, sizeof(raw_address));
    memcpy(bytes + rpmsg_name_size + sizeof(raw_address), &raw_flags, sizeof(raw_flags));
    return castle::status::ok;
}

} // namespace rpmsg
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_RPMSG_NAME_SERVICE_HPP
