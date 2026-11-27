// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file decoder_registry.hpp
 * @brief UBX-specific decoder registry facade for Castle.
 *
 * This header binds the protocol-agnostic decoder infrastructure to
 * `castle::protocols::ubx::message_view`, so UBX users do not need to
 * repeat the raw-view type in every decoder or registry declaration.
 *
 * The implementation remains in `castle_ext/parsers/decoder_registry.hpp`.
 * This layer intentionally contains only type aliases and a zero-overhead
 * `attach()` adapter, making it straightforward to add equivalent facades
 * for future protocols without duplicating dispatch machinery.
 *
 * @code
 * using nav_pvt_decoder = castle::parsers::ubx_parser::message_decoder<
 *     castle::protocols::ubx::messages::nav_pvt>;
 *
 * castle::parsers::ubx_parser::decoder_registry<nav_pvt_decoder> registry;
 *
 * auto connection = registry.decoder<
 *     castle::protocols::ubx::messages::nav_pvt>()
 *     .connect([](const auto& pvt) { (void)pvt; });
 *
 * castle::parsers::ubx_parser::attach(parser, registry);
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

namespace castle
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
using message_decoder = ::castle::parsers::message_decoder<
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

CASTLE_EXT_UBX_DECODER_ALIAS(ack_ack_decoder,          protocols::ubx::messages::ack_ack)
CASTLE_EXT_UBX_DECODER_ALIAS(ack_nak_decoder,          protocols::ubx::messages::ack_nak)
CASTLE_EXT_UBX_DECODER_ALIAS(cfg_spt_decoder,          protocols::ubx::messages::cfg_spt)
CASTLE_EXT_UBX_DECODER_ALIAS(cfg_valget_decoder,       protocols::ubx::messages::cfg_valget<>)
CASTLE_EXT_UBX_DECODER_ALIAS(esf_alg_decoder,          protocols::ubx::messages::esf_alg)
CASTLE_EXT_UBX_DECODER_ALIAS(esf_ins_decoder,          protocols::ubx::messages::esf_ins)
CASTLE_EXT_UBX_DECODER_ALIAS(esf_meas_decoder,         protocols::ubx::messages::esf_meas<>)
CASTLE_EXT_UBX_DECODER_ALIAS(esf_raw_decoder,          protocols::ubx::messages::esf_raw<>)
CASTLE_EXT_UBX_DECODER_ALIAS(esf_status_decoder,       protocols::ubx::messages::esf_status<>)
CASTLE_EXT_UBX_DECODER_ALIAS(inf_decoder,              protocols::ubx::messages::inf<>)
CASTLE_EXT_UBX_DECODER_ALIAS(mga_ack_decoder,          protocols::ubx::messages::mga_ack)
CASTLE_EXT_UBX_DECODER_ALIAS(mga_dbd_decoder,          protocols::ubx::messages::mga_dbd<>)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_comms_decoder,        protocols::ubx::messages::mon_comms<>)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_gnss_decoder,         protocols::ubx::messages::mon_gnss)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_hw_decoder,           protocols::ubx::messages::mon_hw)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_hw2_decoder,          protocols::ubx::messages::mon_hw2)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_hw3_decoder,          protocols::ubx::messages::mon_hw3<>)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_io_decoder,           protocols::ubx::messages::mon_io<>)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_msgpp_decoder,        protocols::ubx::messages::mon_msgpp)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_patch_decoder,        protocols::ubx::messages::mon_patch<>)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_rf_decoder,           protocols::ubx::messages::mon_rf<>)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_rxbuf_decoder,        protocols::ubx::messages::mon_rxbuf)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_rxr_decoder,          protocols::ubx::messages::mon_rxr)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_span_decoder,         protocols::ubx::messages::mon_span<>)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_spt_decoder,          protocols::ubx::messages::mon_spt<>)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_txbuf_decoder,        protocols::ubx::messages::mon_txbuf)
CASTLE_EXT_UBX_DECODER_ALIAS(mon_ver_decoder,          protocols::ubx::messages::mon_ver<>)
CASTLE_EXT_UBX_DECODER_ALIAS(nav2_dop_decoder,         protocols::ubx::messages::nav2_dop)
CASTLE_EXT_UBX_DECODER_ALIAS(nav2_eell_decoder,        protocols::ubx::messages::nav2_eell)
CASTLE_EXT_UBX_DECODER_ALIAS(nav2_pvt_decoder,         protocols::ubx::messages::nav2_pvt)
CASTLE_EXT_UBX_DECODER_ALIAS(nav2_timegps_decoder,     protocols::ubx::messages::nav2_timegps)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_att_decoder,          protocols::ubx::messages::nav_att)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_clock_decoder,        protocols::ubx::messages::nav_clock)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_cov_decoder,          protocols::ubx::messages::nav_cov)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_dop_decoder,          protocols::ubx::messages::nav_dop)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_eell_decoder,         protocols::ubx::messages::nav_eell)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_eoe_decoder,          protocols::ubx::messages::nav_eoe)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_geofence_decoder,     protocols::ubx::messages::nav_geofence<>)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_hpposecef_decoder,    protocols::ubx::messages::nav_hpposecef)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_hpposllh_decoder,     protocols::ubx::messages::nav_hpposllh)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_odo_decoder,          protocols::ubx::messages::nav_odo)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_orb_decoder,          protocols::ubx::messages::nav_orb<>)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_posecef_decoder,      protocols::ubx::messages::nav_posecef)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_posllh_decoder,       protocols::ubx::messages::nav_posllh)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_pvt_decoder,          protocols::ubx::messages::nav_pvt)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_relposned_decoder,    protocols::ubx::messages::nav_relposned)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_sat_decoder,          protocols::ubx::messages::nav_sat<>)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_sbas_decoder,         protocols::ubx::messages::nav_sbas<>)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_sig_decoder,          protocols::ubx::messages::nav_sig<>)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_slas_decoder,         protocols::ubx::messages::nav_slas<>)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_status_decoder,       protocols::ubx::messages::nav_status)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_timebds_decoder,      protocols::ubx::messages::nav_timebds)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_timegal_decoder,      protocols::ubx::messages::nav_timegal)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_timeglo_decoder,      protocols::ubx::messages::nav_timeglo)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_timegps_decoder,      protocols::ubx::messages::nav_timegps)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_timels_decoder,       protocols::ubx::messages::nav_timels)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_timeqzss_decoder,     protocols::ubx::messages::nav_timeqzss)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_timeutc_decoder,      protocols::ubx::messages::nav_timeutc)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_velecef_decoder,      protocols::ubx::messages::nav_velecef)
CASTLE_EXT_UBX_DECODER_ALIAS(nav_velned_decoder,       protocols::ubx::messages::nav_velned)
CASTLE_EXT_UBX_DECODER_ALIAS(rxm_measx_decoder,        protocols::ubx::messages::rxm_measx<>)
CASTLE_EXT_UBX_DECODER_ALIAS(rxm_rawx_decoder,         protocols::ubx::messages::rxm_rawx<>)
CASTLE_EXT_UBX_DECODER_ALIAS(rxm_rlm_long_decoder,     protocols::ubx::messages::rxm_rlm_long)
CASTLE_EXT_UBX_DECODER_ALIAS(rxm_rlm_short_decoder,    protocols::ubx::messages::rxm_rlm_short)
CASTLE_EXT_UBX_DECODER_ALIAS(rxm_rtcm_decoder,         protocols::ubx::messages::rxm_rtcm)
CASTLE_EXT_UBX_DECODER_ALIAS(sec_crc_decoder,          protocols::ubx::messages::sec_crc)
CASTLE_EXT_UBX_DECODER_ALIAS(sec_sig_decoder,          protocols::ubx::messages::sec_sig)
CASTLE_EXT_UBX_DECODER_ALIAS(sec_uniqid_decoder,       protocols::ubx::messages::sec_uniqid)
CASTLE_EXT_UBX_DECODER_ALIAS(tim_tm2_decoder,          protocols::ubx::messages::tim_tm2)
CASTLE_EXT_UBX_DECODER_ALIAS(tim_tp_decoder,           protocols::ubx::messages::tim_tp)
CASTLE_EXT_UBX_DECODER_ALIAS(tim_vrfy_decoder,         protocols::ubx::messages::tim_vrfy)
CASTLE_EXT_UBX_DECODER_ALIAS(upd_sos_ack_decoder,      protocols::ubx::messages::upd_sos_ack)
CASTLE_EXT_UBX_DECODER_ALIAS(upd_sos_restored_decoder, protocols::ubx::messages::upd_sos_restored)
#undef CASTLE_EXT_UBX_DECODER_ALIAS

/**
 * @brief Default UBX decoder registry covering every built-in receiver output.
 *
 * Host-to-receiver messages (commands such as UBX-CFG-RST, UBX-RXM-PMREQ,
 * UBX-UPD-SOS save/clear, and MGA assistance inputs other than UBX-MGA-ACK /
 * UBX-MGA-DBD) are encode-only in practice and are not registered here; add
 * their decoders to a custom `decoder_registry<...>` for loop-back use.
 */
template <
    castle::size_type MaxSubscribers = 1U,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
using default_decoder_registry =
    castle::parsers::decoder_registry<
        protocols::ubx::message_view,
        ack_ack_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        ack_nak_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        cfg_spt_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        cfg_valget_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        esf_alg_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        esf_ins_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        esf_meas_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        esf_raw_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        esf_status_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        inf_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mga_ack_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mga_dbd_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_comms_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_gnss_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_hw_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_hw2_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_hw3_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_io_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_msgpp_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_patch_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_rf_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_rxbuf_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_rxr_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_span_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_spt_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_txbuf_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        mon_ver_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav2_dop_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav2_eell_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav2_pvt_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav2_timegps_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_att_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_clock_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_cov_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_dop_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_eell_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_eoe_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_geofence_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_hpposecef_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_hpposllh_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_odo_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_orb_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_posecef_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_posllh_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_pvt_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_relposned_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_sat_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_sbas_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_sig_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_slas_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_status_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_timebds_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_timegal_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_timeglo_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_timegps_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_timels_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_timeqzss_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_timeutc_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_velecef_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        nav_velned_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        rxm_measx_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        rxm_rawx_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        rxm_rlm_long_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        rxm_rlm_short_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        rxm_rtcm_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        sec_crc_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        sec_sig_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        sec_uniqid_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        tim_tm2_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        tim_tp_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        tim_vrfy_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        upd_sos_ack_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        upd_sos_restored_decoder<MaxSubscribers, StorageSize, StorageAlignment>
    >;

/**
 * @brief Fixed, compile-time UBX decoder registry.
 *
 * @tparam Decoders UBX-bound `message_decoder` types.
 */
template <typename... Decoders>
using decoder_registry = ::castle::parsers::decoder_registry<
    raw_view_type,
    Decoders...>;

/**
 * @brief Result returned after dispatching one validated UBX frame.
 */
using dispatch_result = ::castle::parsers::dispatch_result;

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

    ::castle::parsers::attach(parser, registry);
}

} // namespace ubx_parser
} // namespace parsers
} // namespace castle

#endif // CASTLE_EXT_PARSERS_UBX_PARSER_DECODER_REGISTRY_HPP
