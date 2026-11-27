// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file message.hpp
 * @brief Non-owning RX views and move-by-convention handles for zero-copy TX buffers.
 */
#ifndef CASTLE_EXT_IO_RPMSG_MESSAGE_HPP
#define CASTLE_EXT_IO_RPMSG_MESSAGE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/move.hpp"
#include "castle_ext/io/rpmsg/types.hpp"

#include <stdint.h>

namespace castle
{
namespace io
{
namespace rpmsg
{

/** @brief Non-owning view of one received RPMsg payload; valid only during the RX callback. */
struct message_view
{
    address source;
    address destination;
    uint8_t CASTLE_CONST* data;
    size_type size;

    CASTLE_CONSTEXPR message_view(address src = rpmsg_addr_any,
                                  address dst = rpmsg_addr_any,
                                  void CASTLE_CONST* bytes = nullptr,
                                  size_type length = 0U) CASTLE_NOEXCEPT
        : source(src)
        , destination(dst)
        , data(static_cast<uint8_t CASTLE_CONST*>(bytes))
        , size(length)
    {
    }

    /** @brief Returns true when the payload is empty. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size == 0U; }
};

/**
 * @brief Handle for a payload buffer borrowed from the OpenAMP TX pool.
 *
 * The buffer remains owned by the endpoint/backend until `send_nocopy()` or
 * `release_tx_buffer()` succeeds. Never copy or reuse an active handle.
 */
struct tx_buffer
{
    uint8_t* data;
    size_type capacity;
    void* endpoint_token;
    bool active;

    CASTLE_CONSTEXPR tx_buffer() CASTLE_NOEXCEPT
        : data(nullptr)
        , capacity(0U)
        , endpoint_token(nullptr)
        , active(false)
    {
    }

    tx_buffer(tx_buffer CASTLE_CONST&) CASTLE_DELETE;
    tx_buffer& operator=(tx_buffer CASTLE_CONST&) CASTLE_DELETE;

    tx_buffer(tx_buffer&& other) CASTLE_NOEXCEPT
        : data(other.data)
        , capacity(other.capacity)
        , endpoint_token(other.endpoint_token)
        , active(other.active)
    {
        other.reset();
    }

    tx_buffer& operator=(tx_buffer&&) CASTLE_DELETE;

    /** @brief Clears the local handle after the backend has returned ownership. */
    void reset() CASTLE_NOEXCEPT
    {
        data = nullptr;
        capacity = 0U;
        endpoint_token = nullptr;
        active = false;
    }
};

} // namespace rpmsg
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_RPMSG_MESSAGE_HPP
