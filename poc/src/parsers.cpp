#include "gnss/components.hpp"

namespace gnss
{

castle::geodetic::geo_point<double> ChinaCoordinateService::consumer_point(
    double latitude, double longitude, bool& shifted) const noexcept
{
    const castle::geodetic::geo_point<double> wgs84(latitude, longitude);
    shifted = castle::geodetic::is_in_gcj02_region(wgs84);
    return shifted ? castle::geodetic::wgs84_to_gcj02(wgs84) : wgs84;
}

castle::status ChinaCoordinateService::init() noexcept
{
    return castle::status::ok;
}

castle::status ChinaCoordinateService::deinit() noexcept
{
    return castle::status::ok;
}

castle::status ChinaCoordinateService::suspend() noexcept
{
    return castle::status::ok;
}

castle::status ChinaCoordinateService::resume() noexcept
{
    return castle::status::ok;
}

castle::status ChinaCoordinateService::shutdown() noexcept
{
    return castle::status::ok;
}

UbxDecodeNode::UbxDecodeNode(UbxParserNode& parser, UbxController& controller,
                             DataStore& data, ChinaCoordinateService& china) noexcept
    : parser_(parser)
    , controller_(controller)
    , data_(data)
    , china_(china)
{
}

castle::status UbxDecodeNode::init() noexcept
{
    pvt_connection_ = registry_.decoder<Pvt>().connect(
        [this](const Pvt& pvt) noexcept
        {
            on_pvt(pvt);
        });
    valget_connection_ = registry_.decoder<Valget>().connect(
        [this](const Valget& current) noexcept
        {
            on_valget(current);
        });
    ack_connection_ = registry_.decoder<castle::protocols::ubx::messages::ack_ack>().connect(
        [this](const castle::protocols::ubx::messages::ack_ack& ack) noexcept
        { on_ack_ack(ack); });
    nak_connection_ = registry_.decoder<castle::protocols::ubx::messages::ack_nak>().connect(
        [this](const castle::protocols::ubx::messages::ack_nak& ack) noexcept
        { on_ack_nak(ack); });
    castle::parsers::ubx_parser::attach(parser_.parser(), registry_);
    return castle::status::ok;
}

castle::status UbxDecodeNode::deinit() noexcept
{
    pvt_connection_.disconnect();
    valget_connection_.disconnect();
    ack_connection_.disconnect();
    nak_connection_.disconnect();
    return castle::status::ok;
}

castle::status UbxDecodeNode::suspend() noexcept
{
    return castle::status::ok;
}

castle::status UbxDecodeNode::resume() noexcept
{
    return castle::status::ok;
}

castle::status UbxDecodeNode::shutdown() noexcept
{
    return deinit();
}

void UbxDecodeNode::on_pvt(const Pvt& pvt) noexcept
{
    if (pvt.valid_date() || pvt.valid_time())
    {
        TimeData time{};
        time.year = pvt.year;
        time.month = pvt.month;
        time.day = pvt.day;
        time.hour = pvt.hour;
        time.minute = pvt.minute;
        time.second = pvt.second;
        time.nanoseconds = pvt.nano;
        time.valid = pvt.valid_date() && pvt.valid_time();
        static_cast<void>(data_.pool.publish(time));
    }

    SatelliteData satellites{};
    satellites.used = pvt.num_sv;
    satellites.position_dop_x100 = pvt.p_dop;
    static_cast<void>(data_.pool.publish(satellites));

    if (!pvt.fix_ok() || pvt.invalid_llh() ||
        (pvt.fix_type != castle::protocols::ubx::messages::nav_pvt_fix_type::fix_2d &&
         pvt.fix_type != castle::protocols::ubx::messages::nav_pvt_fix_type::fix_3d &&
         pvt.fix_type != castle::protocols::ubx::messages::nav_pvt_fix_type::gnss_dr))
    {
        return;
    }

    PvtHeartbeat heartbeat{};
    heartbeat.sequence = ++pvt_sequence_;
    static_cast<void>(data_.pool.publish(heartbeat));

    LocationData location{};
    location.wgs84_latitude_deg = pvt.latitude_deg();
    location.wgs84_longitude_deg = pvt.longitude_deg();
    bool shifted = false;
    const auto consumer = china_.consumer_point(
        location.wgs84_latitude_deg, location.wgs84_longitude_deg, shifted);
    location.latitude_deg = consumer.latitude();
    location.longitude_deg = consumer.longitude();
    location.shifted_to_gcj02 = shifted;
    location.altitude_m = pvt.h_msl_m();
    location.horizontal_accuracy_mm = pvt.h_acc;
    location.fix_type = static_cast<uint8_t>(pvt.fix_type);
    location.satellites_used = pvt.num_sv;
    location.source = LocationSource::ubx_pvt;
    location.valid = true;
    static_cast<void>(data_.pool.publish(location));
}

void UbxDecodeNode::on_valget(const Valget& current) noexcept
{
    static_cast<void>(controller_.on_valget(current));
}

void UbxDecodeNode::on_ack_ack(
    const castle::protocols::ubx::messages::ack_ack& ack) noexcept
{
    controller_.on_ack(true, ack.cls_id, ack.acked_msg_id);
}

void UbxDecodeNode::on_ack_nak(
    const castle::protocols::ubx::messages::ack_nak& ack) noexcept
{
    controller_.on_ack(false, ack.cls_id, ack.acked_msg_id);
}

NmeaDecodeNode::NmeaDecodeNode(NmeaParserNode& parser, DataStore& data,
                               ChinaCoordinateService& china) noexcept
    : parser_(parser)
    , data_(data)
    , china_(china)
{
}

castle::status NmeaDecodeNode::init() noexcept
{
    gga_connection_ = registry_.decoder<Gga>().connect(
        [this](const Gga& gga) noexcept
        {
            on_gga(gga);
        });
    castle::parsers::nmea_parser::attach(parser_.parser(), registry_);
    return castle::status::ok;
}

castle::status NmeaDecodeNode::deinit() noexcept
{
    gga_connection_.disconnect();
    return castle::status::ok;
}

castle::status NmeaDecodeNode::suspend() noexcept
{
    return castle::status::ok;
}

castle::status NmeaDecodeNode::resume() noexcept
{
    return castle::status::ok;
}

castle::status NmeaDecodeNode::shutdown() noexcept
{
    return deinit();
}

void NmeaDecodeNode::on_gga(const Gga& gga) noexcept
{
    if (!gga.valid || !gga.fix_available())
    {
        return;
    }
    LocationData location{};
    location.wgs84_latitude_deg = gga.latitude;
    location.wgs84_longitude_deg = gga.longitude;
    bool shifted = false;
    const auto consumer = china_.consumer_point(gga.latitude, gga.longitude, shifted);
    location.latitude_deg = consumer.latitude();
    location.longitude_deg = consumer.longitude();
    location.shifted_to_gcj02 = shifted;
    location.altitude_m = gga.altitude_msl;
    location.satellites_used = gga.num_satellites;
    location.source = LocationSource::nmea_gga;
    location.valid = true;
    static_cast<void>(data_.pool.publish(location));

    SatelliteData satellites{};
    satellites.used = gga.num_satellites;
    satellites.position_dop_x100 = static_cast<uint16_t>(gga.hdop * 100.0);
    static_cast<void>(data_.pool.publish(satellites));
}

} // namespace gnss
