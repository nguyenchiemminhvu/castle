#include "sample_support.hpp"

#include "castle_ext/data/data_pool.hpp"
#include "castle_ext/data/update_dispatcher.hpp"

#include <stdint.h>

namespace
{
struct imu_sample
{
    float accel_z = 0.0F;
};

struct gnss_fix
{
    double latitude = 0.0;
};

struct battery_state
{
    uint16_t millivolts = 0U;
};

using sensor_pool = castle_ext::data::data_pool<2U, imu_sample, gnss_fix, battery_state>;

// Reacts to IMU and GNSS updates as soon as they are published. It has no thread of its own:
// the callbacks run in the publisher's context, so they only read the pool and record results.
class crash_monitor
{
public:
    explicit crash_monitor(const sensor_pool& pool) noexcept
        : pool_(pool)
    {
        dispatcher_.bind<imu_sample>([this]() noexcept
        {
            on_imu();
        });
        dispatcher_.bind<gnss_fix>([this]() noexcept
        {
            on_gnss();
        });
        // battery_state is not bound: its notifications are ignored.
    }

    sensor_pool::observer_type* observer() noexcept
    {
        return &dispatcher_;
    }

    uint32_t imu_events() const noexcept { return imu_events_; }
    uint32_t impacts() const noexcept { return impacts_; }
    double last_latitude() const noexcept { return last_latitude_; }

private:
    void on_imu() noexcept
    {
        ++imu_events_;
        const castle_ext::data::snapshot<imu_sample> sample = pool_.get_snapshot<imu_sample>();
        if (sample->accel_z > 40.0F)
        {
            ++impacts_;
        }
    }

    void on_gnss() noexcept
    {
        last_latitude_ = pool_.get_snapshot<gnss_fix>()->latitude;
    }

    const sensor_pool& pool_;
    sensor_pool::dispatcher_type dispatcher_;
    uint32_t imu_events_ = 0U;
    uint32_t impacts_ = 0U;
    double last_latitude_ = 0.0;
};
} // namespace

int main()
{
    sensor_pool pool;
    crash_monitor monitor(pool);
    CASTLE_SAMPLE_CHECK(pool.add_observer(monitor.observer()));

    // HAL context: each publish() runs the matching callback before it returns.
    CASTLE_SAMPLE_CHECK(pool.publish(imu_sample{9.81F}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(monitor.imu_events() == 1U);
    CASTLE_SAMPLE_CHECK(monitor.impacts() == 0U);

    // No coalescing: every publication is delivered.
    CASTLE_SAMPLE_CHECK(pool.publish(imu_sample{55.0F}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(pool.publish(imu_sample{9.81F}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(monitor.imu_events() == 3U);
    CASTLE_SAMPLE_CHECK(monitor.impacts() == 1U);

    CASTLE_SAMPLE_CHECK(pool.publish(gnss_fix{37.5665}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(monitor.last_latitude() == 37.5665);

    // Unbound type: nothing runs.
    CASTLE_SAMPLE_CHECK(pool.publish(battery_state{12600U}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(monitor.imu_events() == 3U);

    CASTLE_SAMPLE_CHECK(pool.remove_observer(monitor.observer()));
    return 0;
}
