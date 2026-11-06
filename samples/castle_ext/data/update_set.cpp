#include "sample_support.hpp"

#include "castle_ext/data/update_set.hpp"

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
} // namespace

int main()
{
    using updates = castle_ext::data::update_set<imu_sample, gnss_fix, battery_state>;

    CASTLE_SAMPLE_CHECK(updates::capacity() == 3U);

    // Building sets.
    updates changed;
    CASTLE_SAMPLE_CHECK(changed.empty());
    changed.insert<imu_sample>();
    CASTLE_SAMPLE_CHECK(changed.contains<imu_sample>());
    CASTLE_SAMPLE_CHECK(changed.size() == 1U);
    CASTLE_SAMPLE_CHECK(changed == updates::of<imu_sample>());

    // A service merges updates from several producers ...
    changed |= updates::of<gnss_fix, battery_state>();
    CASTLE_SAMPLE_CHECK(changed == updates::all());

    // ... and forwards to an application only the types it is interested in.
    const updates navigation_interest = updates::of<imu_sample, gnss_fix>();
    const updates for_navigation = changed & navigation_interest;
    CASTLE_SAMPLE_CHECK(for_navigation.size() == 2U);
    CASTLE_SAMPLE_CHECK(!for_navigation.contains<battery_state>());
    CASTLE_SAMPLE_CHECK((for_navigation | updates::of<battery_state>()) == changed);

    // Typed dispatch over the changed types, in declaration order.
    uint32_t visited_imu = 0U;
    uint32_t visited_gnss = 0U;
    uint32_t visited_battery = 0U;
    for_navigation.for_each([&](auto type)
    {
        using changed_type = typename decltype(type)::type;
        if (castle::meta::is_same<changed_type, imu_sample>::value)
        {
            ++visited_imu;
        }
        else if (castle::meta::is_same<changed_type, gnss_fix>::value)
        {
            ++visited_gnss;
        }
        else
        {
            ++visited_battery;
        }
    });
    CASTLE_SAMPLE_CHECK(visited_imu == 1U);
    CASTLE_SAMPLE_CHECK(visited_gnss == 1U);
    CASTLE_SAMPLE_CHECK(visited_battery == 0U);

    // Removing types.
    changed.erase<imu_sample, gnss_fix>();
    CASTLE_SAMPLE_CHECK(changed == updates::of<battery_state>());
    CASTLE_SAMPLE_CHECK(changed != for_navigation);
    changed.clear();
    CASTLE_SAMPLE_CHECK(changed.empty());

    return 0;
}
