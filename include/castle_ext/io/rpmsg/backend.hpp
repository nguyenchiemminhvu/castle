// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file backend.hpp
 * @brief Compile-time RPMsg backend contract; no inheritance or virtual dispatch.
 *
 * A backend owns the platform/OpenAMP resources and implements the methods
 * documented by `has_backend_contract`. Endpoint receive and NS callbacks use
 * plain function pointers plus caller context, keeping storage bounded.
 */
#ifndef CASTLE_EXT_IO_RPMSG_BACKEND_HPP
#define CASTLE_EXT_IO_RPMSG_BACKEND_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/io/rpmsg/message.hpp"
#include "castle_ext/io/rpmsg/name_service.hpp"
#include "castle_ext/io/rpmsg/types.hpp"

namespace castle
{
namespace io
{
namespace rpmsg
{

using receive_dispatch = castle::status (*)(void*, message_view CASTLE_CONST&);
using unbind_dispatch = void (*)(void*);
using name_service_dispatch = void (*)(void*, name_service_event CASTLE_CONST&);

namespace detail
{

template <typename Backend, typename = void>
struct has_backend_contract : castle::meta::false_type {};

template <typename Backend>
struct has_backend_contract<Backend, castle::meta::void_t<
    decltype(castle::meta::declval<Backend&>().initialize(
        castle::meta::declval<device_config CASTLE_CONST&>(), castle::meta::declval<void*>(),
        castle::meta::declval<name_service_dispatch>())),
    decltype(castle::meta::declval<Backend&>().shutdown()),
    decltype(castle::meta::declval<Backend CASTLE_CONST&>().is_initialized()),
    decltype(castle::meta::declval<Backend&>().create_endpoint(
        castle::meta::declval<char CASTLE_CONST*>(), castle::meta::declval<address>(),
        castle::meta::declval<address>(), castle::meta::declval<void*>(),
        castle::meta::declval<receive_dispatch>(), castle::meta::declval<unbind_dispatch>(),
        castle::meta::declval<void*&>(), castle::meta::declval<address&>())),
    decltype(castle::meta::declval<Backend&>().destroy_endpoint(castle::meta::declval<void*>())),
    decltype(castle::meta::declval<Backend&>().send(
        castle::meta::declval<void*>(), castle::meta::declval<address>(),
        castle::meta::declval<address>(), castle::meta::declval<void CASTLE_CONST*>(),
        castle::meta::declval<size_type>(), castle::meta::declval<timeout_type>())),
    decltype(castle::meta::declval<Backend&>().acquire_tx_buffer(
        castle::meta::declval<void*>(), castle::meta::declval<bool>(),
        castle::meta::declval<void*&>(), castle::meta::declval<size_type&>())),
    decltype(castle::meta::declval<Backend&>().send_tx_buffer(
        castle::meta::declval<void*>(), castle::meta::declval<address>(),
        castle::meta::declval<address>(), castle::meta::declval<void*>(), castle::meta::declval<size_type>())),
    decltype(castle::meta::declval<Backend&>().release_tx_buffer(
        castle::meta::declval<void*>(), castle::meta::declval<void*>())),
    decltype(castle::meta::declval<Backend&>().remote_address(castle::meta::declval<void*>())),
    decltype(castle::meta::declval<Backend&>().poll())>> : castle::meta::true_type {};

} // namespace detail

} // namespace rpmsg
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_RPMSG_BACKEND_HPP
