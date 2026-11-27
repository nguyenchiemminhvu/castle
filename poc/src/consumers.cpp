#include "gnss/components.hpp"

#include <unistd.h>

namespace gnss
{
namespace
{

void emit_line(const char* data, castle::size_type size) noexcept
{
    if (data != nullptr && size != 0U)
    {
        static_cast<void>(::write(STDOUT_FILENO, data, size));
    }
}

} // namespace

LocationLogger::LocationLogger(DataStore& data) noexcept
    : data_(data)
{
}

castle::status LocationLogger::init() noexcept
{
    return data_.pool.add_observer(&inbox_)
        ? castle::status::ok : castle::status::resource_exhausted;
}

castle::status LocationLogger::deinit() noexcept
{
    static_cast<void>(data_.pool.remove_observer(&inbox_));
    return castle::status::ok;
}

castle::status LocationLogger::suspend() noexcept
{
    return castle::status::ok;
}

castle::status LocationLogger::resume() noexcept
{
    return castle::status::ok;
}

castle::status LocationLogger::shutdown() noexcept
{
    return deinit();
}

void LocationLogger::update() noexcept
{
    const Pool::update_set_type changed = inbox_.take();
    if (!changed.contains<LocationData>())
    {
        return;
    }
    auto location = data_.pool.get_snapshot<LocationData>();
    if (!location || !location->valid)
    {
        return;
    }

    castle::string_builder<160U> line;
    line.build("GNSS src=", static_cast<uint8_t>(location->source),
               " lat=", location->latitude_deg, 6U,
               " lon=", location->longitude_deg, 6U,
               " alt_m=", location->altitude_m, 1U,
               " sv=", location->satellites_used,
               " gcj02=", location->shifted_to_gcj02, "\n");
    emit_line(line.c_str(), line.size());
}

Pearlchain::Pearlchain(DataStore& data, WorkerPool& workers) noexcept
    : data_(data)
    , workers_(workers)
{
}

castle::status Pearlchain::init() noexcept
{
    return data_.pool.add_observer(&inbox_)
        ? castle::status::ok : castle::status::resource_exhausted;
}

castle::status Pearlchain::deinit() noexcept
{
    static_cast<void>(data_.pool.remove_observer(&inbox_));
    return castle::status::ok;
}

castle::status Pearlchain::suspend() noexcept
{
    return castle::status::ok;
}

castle::status Pearlchain::resume() noexcept
{
    return castle::status::ok;
}

castle::status Pearlchain::shutdown() noexcept
{
    return deinit();
}

void Pearlchain::update(uint32_t now_ms) noexcept
{
    const Pool::update_set_type changed = inbox_.take();
    if (!changed.contains<LocationData>())
    {
        return;
    }
    const auto sample = data_.pool.get_snapshot<LocationData>();
    if (!sample || !sample->valid)
    {
        return;
    }

    const castle::geodetic::geo_point<double> point(
        sample->latitude_deg, sample->longitude_deg);
    if (!have_last_)
    {
        last_ = Waypoint{point.latitude(), point.longitude(), now_ms};
        have_last_ = true;
        static_cast<void>(waypoints_.push(last_));
        return;
    }

    const castle::geodetic::geo_point<double> previous(
        last_.latitude_deg, last_.longitude_deg);
    const double distance_m = castle::geodetic::haversine_distance(previous, point);
    if (distance_m < 50.0)
    {
        return;
    }

    last_ = Waypoint{point.latitude(), point.longitude(), now_ms};
    static_cast<void>(waypoints_.force_push(last_));
    const Waypoint stored = last_;
    static_cast<void>(workers_.submit([stored]() noexcept
    {
        castle::string_builder<128U> line;
        line.build("PEARL waypoint lat=", stored.latitude_deg, 6U,
                   " lon=", stored.longitude_deg, 6U,
                   " t_ms=", stored.monotonic_ms, "\n");
        static_cast<void>(::write(STDOUT_FILENO, line.c_str(), line.size()));
    }));
}

castle::size_type Pearlchain::waypoint_count() noexcept
{
    return waypoints_.size();
}

} // namespace gnss
