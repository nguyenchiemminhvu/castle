#include "sample_support.hpp"

#include "castle_ext/data/update_inbox.hpp"

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
};

using updates = castle_ext::data::update_set<imu_sample, gnss_fix>;
using inbox_type = castle_ext::data::update_inbox<imu_sample, gnss_fix>;
} // namespace

int main()
{
    // The consumer thread sleeps on a semaphore; the inbox releases it when updates arrive.
    castle::semaphore<1> wake_up(0U);
    uint32_t wake_count = 0U;
    inbox_type app_inbox{[&wake_up, &wake_count]() noexcept
    {
        ++wake_count;
        (void)wake_up.release();
    }};

    // Producers usually reach the inbox through an observable (a data_pool or a service).
    castle::patterns::observable<castle::patterns::observer<updates>, 2U> service;
    CASTLE_SAMPLE_CHECK(service.add_observer(&app_inbox));
    CASTLE_SAMPLE_CHECK(!app_inbox.has_pending());

    // Two notifications before the consumer runs: coalesced, one wake-up.
    service.notify_observers(updates::of<imu_sample>());
    service.notify_observers(updates::of<imu_sample, gnss_fix>());
    CASTLE_SAMPLE_CHECK(app_inbox.has_pending());
    CASTLE_SAMPLE_CHECK(wake_count == 1U);

    // Consumer thread: wait, then drain everything at once.
    CASTLE_SAMPLE_CHECK(wake_up.try_acquire());
    const updates changed = app_inbox.take();
    CASTLE_SAMPLE_CHECK(changed == updates::all());
    CASTLE_SAMPLE_CHECK(!app_inbox.has_pending());
    CASTLE_SAMPLE_CHECK(app_inbox.take().empty());

    // The next notification after a drain wakes the consumer again.
    app_inbox.notify(updates::of<gnss_fix>());
    CASTLE_SAMPLE_CHECK(wake_count == 2U);
    CASTLE_SAMPLE_CHECK(app_inbox.take() == updates::of<gnss_fix>());

    // Without a wake hook the consumer polls has_pending().
    inbox_type polled_inbox;
    polled_inbox.notify(updates::of<imu_sample>());
    CASTLE_SAMPLE_CHECK(polled_inbox.has_pending());
    CASTLE_SAMPLE_CHECK(polled_inbox.take().contains<imu_sample>());

    CASTLE_SAMPLE_CHECK(service.remove_observer(&app_inbox));
    return 0;
}
