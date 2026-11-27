#include "gnss/components.hpp"

namespace gnss
{

GnssReceiver::GnssReceiver(GnssTransport& transport, UbxParserNode& ubx,
                           NmeaParserNode& nmea) noexcept
    : transport_(transport)
    , ubx_(ubx)
    , nmea_(nmea)
{
}

castle::status GnssReceiver::init() noexcept
{
    return transport_.is_open() ? castle::status::ok : castle::status::not_configured;
}

castle::status GnssReceiver::deinit() noexcept
{
    return castle::status::ok;
}

castle::status GnssReceiver::suspend() noexcept
{
    return castle::status::ok;
}

castle::status GnssReceiver::resume() noexcept
{
    return castle::status::ok;
}

castle::status GnssReceiver::shutdown() noexcept
{
    return castle::status::ok;
}

castle::status GnssReceiver::poll(uint32_t& received) noexcept
{
    received = 0U;
    castle::size_type count = 0U;
    const castle::status result = transport_.read(rx_buffer_.data(), rx_buffer_.size(), count);
    if (result != castle::status::ok)
    {
        return result;
    }
    if (count == 0U)
    {
        return castle::status::empty;
    }

    // One UART owner feeds both independent framing state machines in a stable order.
    ubx_.feed(rx_buffer_.data(), count);
    nmea_.feed(rx_buffer_.data(), count);
    received = static_cast<uint32_t>(count);
    return castle::status::ok;
}

} // namespace gnss
