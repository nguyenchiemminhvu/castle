#pragma once

#include "gnss/types.hpp"
#include "castle/events/thread_pool.hpp"
#include "castle/dsa/array.hpp"
#include "castle/events/event_handler.hpp"
#include "castle/events/tick_timer.hpp"
#include "castle/sync/atomic.hpp"
#include "castle/patterns/object_graph.hpp"
#include "castle/patterns/action_sequence.hpp"
#include "castle_ext/diag/diagnostic_manager.hpp"
#include "castle_ext/io/uart/uart.hpp"
#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"
#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"
#include "castle_ext/parsers/ubx_parser/decoder_registry.hpp"
#include "castle_ext/parsers/nmea_parser/decoder_registry.hpp"
#include "castle_ext/protocols/ubx/messages/messages.hpp"
#include "castle_ext/protocols/nmea/messages/messages.hpp"
#include "castle_ext/geodetic/coord.hpp"
#include "castle_ext/geodetic/distance.hpp"
#include "castle_ext/geodetic/transform.hpp"
#include "castle_ext/serialization/ini.hpp"
#include "castle/dsa/map.hpp"
#include "castle/dsa/ring_buffer.hpp"
#include "castle/utility/string_builder.hpp"
#include "castle_ext/io/fd/poll.hpp"
#include "castle_ext/io/fd/descriptor.hpp"

#include <stdint.h>
#include <time.h>

namespace gnss
{

class WorkerPool : public castle::events::thread_pool<1U, 16U, 96U, alignof(void*)>
{
public:
    castle::status init() noexcept
    {
        return running() ? castle::status::ok : castle::status::system_call_error;
    }
    castle::status deinit() noexcept
    {
        return shutdown();
    }
    castle::status suspend() noexcept
    {
        return castle::status::ok;
    }
    castle::status resume() noexcept
    {
        return castle::status::ok;
    }
    castle::status shutdown() noexcept
    {
        return this->stop() ? castle::status::ok : castle::status::system_call_error;
    }
};

class ChinaCoordinateService
{
public:
    castle::geodetic::geo_point<double> consumer_point(double latitude, double longitude,
                                                       bool& shifted) const noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
};

class GnssTransport
{
public:
    explicit GnssTransport(const GnssTransConfig& config) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    castle::status reopen() noexcept;
    castle::status read(uint8_t* bytes, castle::size_type capacity,
                        castle::size_type& count) noexcept;
    castle::status write(const uint8_t* bytes, castle::size_type size,
                         castle::size_type& count) noexcept;
    bool is_open() const noexcept;
    int native_handle() const noexcept;
private:
    const GnssTransConfig& config_;
    castle::io::uart::uart uart_{};
    bool suspended_ = false;
};

class UbxController
{
public:
    using ConfigMap = castle::dsa::map<uint32_t, uint64_t, 64U>;
    UbxController(const GnssTransConfig& config, GnssTransport& transport) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    castle::status begin_boot_sync() noexcept;
    castle::status on_valget(const castle::protocols::ubx::messages::cfg_valget<>& current) noexcept;
    void on_ack(bool accepted, uint8_t msg_class, uint8_t msg_id) noexcept;
    castle::status cold_start() noexcept;
    const ConfigMap& desired() const noexcept;
    uint8_t sync_state() const noexcept;
    uint32_t differing_items() const noexcept;
private:
    castle::status load_config() noexcept;
    castle::status send_message(const castle::protocols::ubx::messages::cfg_valget_poll<>& message) noexcept;
    template <typename Message>
    castle::status send_ubx(const Message& message) noexcept;
    const GnssTransConfig& config_;
    GnssTransport& transport_;
    ConfigMap desired_{};
    uint8_t sync_state_ = 0U; // 0=not started, 1=waiting VALGET, 2=waiting ACK, 3=synced, 4=failed
    uint32_t differing_items_ = 0U;
    bool restart_sync_on_resume_ = false;
};

class UbxParserNode
{
public:
    using Parser = castle::parsers::ubx_parser::ubx_parser<>;
    Parser& parser() noexcept
    {
        return parser_;
    }
    void feed(const uint8_t* data, castle::size_type size) noexcept
    {
        parser_.feed(data, size);
    }
    castle::status init() noexcept
    {
        return castle::status::ok;
    }
    castle::status deinit() noexcept
    {
        parser_.reset();
        return castle::status::ok;
    }
    castle::status suspend() noexcept
    {
        parser_.reset();
        return castle::status::ok;
    }
    castle::status resume() noexcept
    {
        return castle::status::ok;
    }
    castle::status shutdown() noexcept
    {
        parser_.reset();
        return castle::status::ok;
    }
private:
    Parser parser_{};
};

class NmeaParserNode
{
public:
    using Parser = castle::parsers::nmea_parser::nmea_parser<>;
    Parser& parser() noexcept
    {
        return parser_;
    }
    void feed(const uint8_t* data, castle::size_type size) noexcept
    {
        parser_.feed(data, size);
    }
    castle::status init() noexcept
    {
        return castle::status::ok;
    }
    castle::status deinit() noexcept
    {
        parser_.reset();
        return castle::status::ok;
    }
    castle::status suspend() noexcept
    {
        parser_.reset();
        return castle::status::ok;
    }
    castle::status resume() noexcept
    {
        return castle::status::ok;
    }
    castle::status shutdown() noexcept
    {
        parser_.reset();
        return castle::status::ok;
    }
private:
    Parser parser_{};
};

class UbxDecodeNode
{
public:
    using Pvt = castle::protocols::ubx::messages::nav_pvt;
    using Valget = castle::protocols::ubx::messages::cfg_valget<>;
    using AckAckDecoder = castle::parsers::ubx_parser::ack_ack_decoder<>;
    using AckNakDecoder = castle::parsers::ubx_parser::ack_nak_decoder<>;
    using PvtDecoder = castle::parsers::ubx_parser::nav_pvt_decoder<>;
    using ValgetDecoder = castle::parsers::ubx_parser::cfg_valget_decoder<>;
    using Registry = castle::parsers::ubx_parser::decoder_registry<
        PvtDecoder, ValgetDecoder, AckAckDecoder, AckNakDecoder>;
    UbxDecodeNode(UbxParserNode& parser, UbxController& controller,
                  DataStore& data, ChinaCoordinateService& china) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
private:
    void on_pvt(const Pvt& pvt) noexcept;
    void on_valget(const Valget& current) noexcept;
    void on_ack_ack(const castle::protocols::ubx::messages::ack_ack& ack) noexcept;
    void on_ack_nak(const castle::protocols::ubx::messages::ack_nak& ack) noexcept;
    UbxParserNode& parser_;
    UbxController& controller_;
    DataStore& data_;
    ChinaCoordinateService& china_;
    Registry registry_{};
    PvtDecoder::connection_type pvt_connection_{};
    ValgetDecoder::connection_type valget_connection_{};
    AckAckDecoder::connection_type ack_connection_{};
    AckNakDecoder::connection_type nak_connection_{};
    uint32_t pvt_sequence_ = 0U;
};

class NmeaDecodeNode
{
public:
    using Gga = castle::protocols::nmea::messages::gga;
    using GgaDecoder = castle::parsers::nmea_parser::gga_decoder<>;
    using Registry = castle::parsers::nmea_parser::decoder_registry<GgaDecoder>;
    NmeaDecodeNode(NmeaParserNode& parser, DataStore& data,
                   ChinaCoordinateService& china) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
private:
    void on_gga(const Gga& gga) noexcept;
    NmeaParserNode& parser_;
    DataStore& data_;
    ChinaCoordinateService& china_;
    Registry registry_{};
    GgaDecoder::connection_type gga_connection_{};
};

class GnssReceiver
{
public:
    GnssReceiver(GnssTransport& transport, UbxParserNode& ubx, NmeaParserNode& nmea) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    castle::status poll(uint32_t& received) noexcept;
private:
    GnssTransport& transport_;
    UbxParserNode& ubx_;
    NmeaParserNode& nmea_;
    castle::dsa::array<uint8_t, 256U> rx_buffer_{};
};

class LocationLogger
{
public:
    explicit LocationLogger(DataStore& data) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    void update() noexcept;
private:
    DataStore& data_;
    Pool::inbox_type inbox_{};
};

class Pearlchain
{
public:
    Pearlchain(DataStore& data, WorkerPool& workers) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    void update(uint32_t now_ms) noexcept;
    castle::size_type waypoint_count() noexcept;
private:
    DataStore& data_;
    WorkerPool& workers_;
    Pool::inbox_type inbox_{};
    castle::dsa::ring_buffer<Waypoint, 128U> waypoints_{};
    Waypoint last_{};
    bool have_last_ = false;
};

struct NoBytesTag {};
struct NoPvtTag {};
struct ReceiverFaultTag {};

class DiagnosticComponent
{
public:
    using Manager = castle::diag::diagnostic_manager<
        castle::diag::diagnostic_event<NoBytesTag>,
        castle::diag::diagnostic_event<NoPvtTag>,
        castle::diag::diagnostic_group<ReceiverFaultTag,
            castle::diag::any_of<castle::diag::active<NoBytesTag>,
                                 castle::diag::active<NoPvtTag>>>>;
    DiagnosticComponent(DataStore& data, WorkerPool& workers) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    void sample(uint32_t now_ms, uint32_t last_byte_ms, uint32_t last_pvt_ms) noexcept;
    uint32_t consume_recovery_requests() noexcept;
    uint8_t recovery_level() const noexcept;
    void mark_recovery_level(uint8_t level) noexcept;
    bool no_bytes_fault() const noexcept;
    bool no_pvt_fault() const noexcept;
private:
    static void recovery_callback(void* context) noexcept;
    void queue_recovery(uint8_t level) noexcept;
    DataStore& data_;
    WorkerPool& workers_;
    castle::events::event_handler<8U> event_handler_{};
    Manager manager_{};
    castle::patterns::action_sequence<2U> recovery_sequence_{};
    castle::atomic<uint32_t> pending_recovery_{0U};
    uint32_t fault_since_ms_ = 0U;
    uint32_t previous_sample_ms_ = 0U;
    uint8_t recovery_level_ = 0U;
    bool no_bytes_ = false;
    bool no_pvt_ = false;
    bool active_ = false;
    bool suspended_ = false;
};

class GnssHal
{
public:
    GnssHal(GnssTransport& transport, UbxController& controller, GnssReceiver& receiver,
            DiagnosticComponent& diagnostic, DataStore& data) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    void update(uint32_t now_ms) noexcept;
    bool running() const noexcept;
private:
    GnssTransport& transport_;
    UbxController& controller_;
    GnssReceiver& receiver_;
    DiagnosticComponent& diagnostic_;
    DataStore& data_;
    Pool::inbox_type inbox_{};
    uint32_t last_byte_ms_ = 0U;
    uint32_t last_pvt_ms_ = 0U;
    uint32_t total_rx_bytes_ = 0U;
    uint8_t recovery_level_ = 0U;
    bool suspended_ = false;
    bool running_ = false;
};

class TickEngine
{
public:
    TickEngine(GnssHal& hal, LocationLogger& logger, Pearlchain& pearlchain) noexcept;
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    void tick(uint32_t now_ms, uint32_t elapsed_ticks = 1U) noexcept;
private:
    GnssHal& hal_;
    LocationLogger& logger_;
    Pearlchain& pearlchain_;
    castle::events::tick_timer<1U> tick_timer_{};
    castle::events::tick_timer<1U>::subscription tick_subscription_{};
    uint32_t now_ms_ = 0U;
};

class PosixTickSource
{
public:
    castle::status init() noexcept;
    castle::status deinit() noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    bool is_open() const noexcept;
private:
    void arm(bool enabled) noexcept;
    timer_t timer_{};
    bool created_ = false;
    bool suspended_ = false;
};

class LocationService
{
public:
    using Graph = castle::patterns::object_graph<
        GnssTransConfig,
        DataStore,
        WorkerPool,
        ChinaCoordinateService,
        castle::patterns::object<GnssTransport, castle::patterns::init_with<GnssTransConfig>>,
        UbxParserNode,
        NmeaParserNode,
        castle::patterns::object<UbxController,
            castle::patterns::uses<GnssTransConfig, GnssTransport>>,
        castle::patterns::object<UbxDecodeNode,
            castle::patterns::uses<UbxParserNode, UbxController, DataStore, ChinaCoordinateService>>,
        castle::patterns::object<NmeaDecodeNode,
            castle::patterns::uses<NmeaParserNode, DataStore, ChinaCoordinateService>>,
        castle::patterns::object<GnssReceiver,
            castle::patterns::uses<GnssTransport, UbxParserNode, NmeaParserNode>>,
        castle::patterns::object<LocationLogger, castle::patterns::uses<DataStore>>,
        castle::patterns::object<Pearlchain, castle::patterns::uses<DataStore, WorkerPool>>,
        castle::patterns::object<DiagnosticComponent, castle::patterns::uses<DataStore, WorkerPool>>,
        castle::patterns::object<GnssHal,
            castle::patterns::uses<GnssTransport, UbxController, GnssReceiver, DiagnosticComponent, DataStore>>,
        castle::patterns::object<TickEngine,
            castle::patterns::uses<GnssHal, LocationLogger, Pearlchain>>,
        PosixTickSource>;

    Graph& graph() noexcept
    {
        return graph_;
    }
    castle::status init() noexcept;
    void update(uint32_t now_ms, uint32_t elapsed_ticks = 1U) noexcept;
    castle::status suspend() noexcept;
    castle::status resume() noexcept;
    castle::status shutdown() noexcept;
    GnssTransConfig& config() noexcept;
    UbxController& controller() noexcept;
    PosixTickSource& tick_source() noexcept;
private:
    Graph graph_{};
    bool suspended_ = false;
    bool initialized_ = false;
};

uint32_t monotonic_milliseconds() noexcept;

} // namespace gnss
