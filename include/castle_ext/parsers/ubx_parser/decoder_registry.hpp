// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file decoder_registry.hpp
 * @brief UBX-specific decoder registry facade for Castle.
 *
 * This header binds the protocol-agnostic decoder infrastructure to
 * `castle_ext::protocols::ubx::message_view`, so UBX users do not need to
 * repeat the raw-view type in every decoder or registry declaration.
 *
 * The implementation remains in `castle_ext/parsers/decoder_registry.hpp`.
 * This layer intentionally contains only type aliases and a zero-overhead
 * `attach()` adapter, making it straightforward to add equivalent facades
 * for future protocols without duplicating dispatch machinery.
 *
 * @code
 * using nav_pvt_decoder = castle_ext::parsers::ubx_parser::message_decoder<
 *     castle_ext::protocols::ubx::messages::nav_pvt>;
 *
 * castle_ext::parsers::ubx_parser::decoder_registry<nav_pvt_decoder> registry;
 *
 * auto connection = registry.decoder<
 *     castle_ext::protocols::ubx::messages::nav_pvt>()
 *     .connect([](const auto& pvt) { (void)pvt; });
 *
 * castle_ext::parsers::ubx_parser::attach(parser, registry);
 * @endcode
 */
#ifndef CASTLE_EXT_PARSERS_UBX_PARSER_DECODER_REGISTRY_HPP
#define CASTLE_EXT_PARSERS_UBX_PARSER_DECODER_REGISTRY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/messages.hpp"
#include "castle_ext/parsers/decoder_registry.hpp"

namespace castle_ext
{
namespace parsers
{
namespace ubx_parser
{

/**
 * @brief Validated raw message view consumed by all UBX decoders.
 */
using raw_view_type = protocols::ubx::message_view;

/**
 * @brief UBX-bound version of the generic message decoder.
 *
 * @tparam Message Decoded UBX message type exposing `matches()` and `decode()`.
 * @tparam MaxSubscribers Maximum simultaneously connected subscribers.
 * @tparam StorageSize Inline storage reserved for each subscriber callback.
 * @tparam StorageAlignment Alignment used for each subscriber callback.
 */
template <
    typename Message,
    castle::size_type MaxSubscribers = 1U,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
using message_decoder = ::castle_ext::parsers::message_decoder<
    Message,
    raw_view_type,
    MaxSubscribers,
    StorageSize,
    StorageAlignment>;

#define CASTLE_EXT_UBX_DECODER_ALIAS(name, type) \
    template < \
        castle::size_type MaxSubscribers = 1U, \
        castle::size_type StorageSize = castle::inplace_storage_reserved, \
        castle::size_type StorageAlignment = castle::inplace_alignment_default> \
    using name = message_decoder<type, MaxSubscribers, StorageSize, StorageAlignment>;

CASTLE_EXT_UBX_DECODER_ALIAS(ack_ack_decoder, protocols::ubx::messages::ack_ack);
CASTLE_EXT_UBX_DECODER_ALIAS(ack_nak_decoder, protocols::ubx::messages::ack_nak);
CASTLE_EXT_UBX_DECODER_ALIAS(cfg_valget_decoder, protocols::ubx::messages::cfg_valget<>);
CASTLE_EXT_UBX_DECODER_ALIAS(esf_ins_decoder, protocols::ubx::messages::esf_ins);
CASTLE_EXT_UBX_DECODER_ALIAS(esf_meas_decoder, protocols::ubx::messages::esf_meas<>);
CASTLE_EXT_UBX_DECODER_ALIAS(esf_status_decoder, protocols::ubx::messages::esf_status<>);
CASTLE_EXT_UBX_DECODER_ALIAS(inf_decoder, protocols::ubx::messages::inf<>);
CASTLE_EXT_UBX_DECODER_ALIAS(mon_io_decoder, protocols::ubx::messages::mon_io<>);
CASTLE_EXT_UBX_DECODER_ALIAS(mon_span_decoder, protocols::ubx::messages::mon_span<>);
CASTLE_EXT_UBX_DECODER_ALIAS(mon_txbuf_decoder, protocols::ubx::messages::mon_txbuf);
CASTLE_EXT_UBX_DECODER_ALIAS(mon_ver_decoder, protocols::ubx::messages::mon_ver<>);
CASTLE_EXT_UBX_DECODER_ALIAS(nav2_dop_decoder, protocols::ubx::messages::nav2_dop);
CASTLE_EXT_UBX_DECODER_ALIAS(nav2_eell_decoder, protocols::ubx::messages::nav2_eell);
CASTLE_EXT_UBX_DECODER_ALIAS(nav2_pvt_decoder, protocols::ubx::messages::nav2_pvt);
CASTLE_EXT_UBX_DECODER_ALIAS(nav2_timegps_decoder, protocols::ubx::messages::nav2_timegps);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_att_decoder, protocols::ubx::messages::nav_att);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_clock_decoder, protocols::ubx::messages::nav_clock);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_dop_decoder, protocols::ubx::messages::nav_dop);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_eell_decoder, protocols::ubx::messages::nav_eell);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_odo_decoder, protocols::ubx::messages::nav_odo);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_pvt_decoder, protocols::ubx::messages::nav_pvt);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_sat_decoder, protocols::ubx::messages::nav_sat<>);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_sig_decoder, protocols::ubx::messages::nav_sig<>);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_status_decoder, protocols::ubx::messages::nav_status);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_timegps_decoder, protocols::ubx::messages::nav_timegps);
CASTLE_EXT_UBX_DECODER_ALIAS(nav_timeutc_decoder, protocols::ubx::messages::nav_timeutc);
CASTLE_EXT_UBX_DECODER_ALIAS(rxm_measx_decoder, protocols::ubx::messages::rxm_measx<>);
CASTLE_EXT_UBX_DECODER_ALIAS(sec_crc_decoder, protocols::ubx::messages::sec_crc);
CASTLE_EXT_UBX_DECODER_ALIAS(sec_sig_decoder, protocols::ubx::messages::sec_sig);
CASTLE_EXT_UBX_DECODER_ALIAS(tim_tp_decoder, protocols::ubx::messages::tim_tp);
CASTLE_EXT_UBX_DECODER_ALIAS(upd_sos_decoder, protocols::ubx::messages::upd_sos_output);
#undef CASTLE_EXT_UBX_DECODER_ALIAS

/** @brief Default UBX decoder registry covering every built-in message type. */
template <
    castle::size_type MaxSubscribers = 1U,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
using default_decoder_registry =
    castle_ext::parsers::decoder_registry<
        protocols::ubx::message_view,
        ack_ack_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        ack_nak_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        cfg_valget_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        esf_ins_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        esf_meas_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        esf_status_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        inf_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_io_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_span_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_txbuf_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_ver_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav2_dop_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav2_eell_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav2_pvt_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav2_timegps_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_att_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_clock_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_dop_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_eell_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_odo_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_pvt_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_sat_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_sig_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_status_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_timegps_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_timeutc_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        rxm_measx_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        sec_crc_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        sec_sig_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        tim_tp_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        upd_sos_decoder<MaxSubscribers, StorageSize, StorageAlignment>
    >;

/**
 * @brief Fixed, compile-time UBX decoder registry.
 *
 * @tparam Decoders UBX-bound `message_decoder` types.
 */
template <typename... Decoders>
using decoder_registry = ::castle_ext::parsers::decoder_registry<
    raw_view_type,
    Decoders...>;

/**
 * @brief Result returned after dispatching one validated UBX frame.
 */
using dispatch_result = ::castle_ext::parsers::dispatch_result;

/**
 * @brief Attaches a UBX-compatible parser to a UBX decoder registry.
 *
 * @tparam Parser A parser exposing `message_type`, `set_message_callback()`,
 *                and whose `message_type` is `protocols::ubx::message_view`.
 * @tparam Registry A `ubx_parser::decoder_registry<...>`.
 *
 * @warning `registry` must outlive the installed parser callback.
 */
template <typename Parser, typename Registry>
void attach(Parser& parser, Registry& registry) CASTLE_NOEXCEPT
{
    static_assert(
        castle::meta::is_same<typename Parser::message_type, raw_view_type>::value,
        "ubx_parser::attach requires a parser whose message_type is UBX message_view");
    static_assert(
        castle::meta::is_same<typename Registry::raw_view_type, raw_view_type>::value,
        "ubx_parser::attach requires a registry bound to UBX message_view");

    ::castle_ext::parsers::attach(parser, registry);
}

} // namespace ubx_parser
} // namespace parsers
} // namespace castle_ext

#endif // CASTLE_EXT_PARSERS_UBX_PARSER_DECODER_REGISTRY_HPP
