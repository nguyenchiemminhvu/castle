#include "sample_support.hpp"

#include "castle_ext/data/data_pool.hpp"

#include "castle/patterns/observer.hpp"
#include "castle/sync/semaphore.hpp"

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
    double longitude = 0.0;
};

struct battery_state
{
    uint16_t millivolts = 0U;
};

// gnss_fix snapshots may be held by two readers at once while new fixes arrive.
using sensor_pool = castle_ext::data::data_pool<
    1U,
    imu_sample,
    castle_ext::data::data_slot<gnss_fix, 2U>,
    battery_state>;

using updates = sensor_pool::update_set_type;

// HAL: writes into the pool; knows nothing about the service or the applications.
class sensor_hal
{
public:
    void attach(sensor_pool& pool)
    {
        pool_ = &pool;
    }

    bool on_imu_interrupt(float accel_z)
    {
        return pool_->publish(imu_sample{accel_z}) == castle::status::ok;
    }

    bool on_gnss_message(double latitude, double longitude)
    {
        return pool_->publish(gnss_fix{latitude, longitude}) == castle::status::ok;
    }

private:
    sensor_pool* pool_ = nullptr;
};

// Application adapter: reads only the types that changed, through a read-only pool reference.
class navigation_app
{
public:
    void attach(const sensor_pool& pool)
    {
        pool_ = &pool;
    }

    sensor_pool::observer_type* inbox()
    {
        return &inbox_;
    }

    // Runs in the application's own thread.
    void step()
    {
        const updates changed = inbox_.take();
        if (changed.contains<imu_sample>())
        {
            last_accel_z = pool_->get_snapshot<imu_sample>()->accel_z;
        }
        if (changed.contains<gnss_fix>())
        {
            const castle_ext::data::snapshot<gnss_fix> fix = pool_->get_snapshot<gnss_fix>();
            last_latitude = fix->latitude;
            fix_generation = fix.generation();
        }
    }

    float last_accel_z = 0.0F;
    double last_latitude = 0.0;
    uint32_t fix_generation = 0U;

private:
    const sensor_pool* pool_ = nullptr;
    sensor_pool::inbox_type inbox_;
};

// Service: owns the pool, hands it to HAL and applications, and propagates updated types.
class sensor_service
{
public:
    sensor_service()
    {
        inbox_.set_wake([this]() noexcept
        {
            (void)wake_.release();
        });
    }

    bool register_data_pool(sensor_hal& hal, navigation_app& app)
    {
        hal.attach(pool_);
        app.attach(pool_);
        return pool_.add_observer(&inbox_) && apps_.add_observer(app.inbox());
    }

    // Runs in the service thread: forward what changed, filtered to what applications need.
    void step()
    {
        const updates changed = inbox_.take() & updates::of<imu_sample, gnss_fix>();
        if (!changed.empty())
        {
            apps_.notify_observers(changed);
        }
    }

    bool has_pending() const
    {
        return inbox_.has_pending();
    }

    // Service thread: blocks until the inbox has work; the wake hook releases this.
    bool try_wait()
    {
        return wake_.try_acquire();
    }

    sensor_pool& pool()
    {
        return pool_;
    }

private:
    castle::semaphore<1> wake_{0U};
    sensor_pool pool_;
    sensor_pool::inbox_type inbox_;
    castle::patterns::observable<sensor_pool::observer_type, 4U> apps_;
};
} // namespace

int main()
{
    sensor_service service;
    sensor_hal hal;
    navigation_app app;
    CASTLE_SAMPLE_CHECK(service.register_data_pool(hal, app));

    // Before any publication every type reads as its default value, generation 0.
    CASTLE_SAMPLE_CHECK(service.pool().get_snapshot<battery_state>().generation() == 0U);

    // HAL thread publishes; the service inbox records which types changed.
    CASTLE_SAMPLE_CHECK(hal.on_imu_interrupt(9.81F));
    CASTLE_SAMPLE_CHECK(hal.on_gnss_message(48.137, 11.575));
    CASTLE_SAMPLE_CHECK(service.pool().publish(battery_state{3700U}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(service.has_pending());

    // The first update woke the service thread; later updates coalesce into the same wake-up.
    CASTLE_SAMPLE_CHECK(service.try_wait());
    CASTLE_SAMPLE_CHECK(!service.try_wait());

    // Service thread forwards { imu_sample, gnss_fix }; battery_state is filtered out.
    service.step();
    CASTLE_SAMPLE_CHECK(!service.has_pending());

    // Application thread reads only the changed types, without copying the pool data.
    app.step();
    CASTLE_SAMPLE_CHECK(app.last_accel_z == 9.81F);
    CASTLE_SAMPLE_CHECK(app.last_latitude == 48.137);
    CASTLE_SAMPLE_CHECK(app.fix_generation == 1U);

    // Battery data is still available to anyone who asks for it.
    CASTLE_SAMPLE_CHECK(service.pool().get_snapshot<battery_state>()->millivolts == 3700U);

    // Only the GNSS fix changes next: the application touches nothing else.
    CASTLE_SAMPLE_CHECK(hal.on_gnss_message(48.138, 11.576));
    CASTLE_SAMPLE_CHECK(service.try_wait());
    service.step();
    app.step();
    CASTLE_SAMPLE_CHECK(app.last_latitude == 48.138);
    CASTLE_SAMPLE_CHECK(app.fix_generation == 2U);
    CASTLE_SAMPLE_CHECK(app.last_accel_z == 9.81F);

    // Observer capacity is fixed at compile time; unknown observers cannot be removed.
    sensor_pool::inbox_type extra_inbox;
    CASTLE_SAMPLE_CHECK(!service.pool().add_observer(&extra_inbox));   // MaxObservers == 1
    CASTLE_SAMPLE_CHECK(!service.pool().remove_observer(&extra_inbox));

    return 0;
}
